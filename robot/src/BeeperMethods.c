#include "BeeperMethods.h"
#include "stepper.h"      /* stepper_isr_step() : TIM1 est partage avec le moteur pas-a-pas */
#include <stdint.h>

/* ==========================================================================
 * Avertisseur sonore sur BUZZER ACTIF (oscillateur interne) -> P2.4 via MOSFET.
 *
 * Le buzzer etant actif, le MC n'a PAS a generer la porteuse : il suffit
 * d'ouvrir/fermer la sortie (enveloppe marche/arret). On produit donc un
 * "bip ... bip ... bip" dont la cadence depend de la distance (cf. proximetre.c).
 *
 * TIM1 est l'unique timer libre du projet et il est PARTAGE :
 *   - tick de base periodique (TICK_US) qui sert a la fois
 *       (1) a faire avancer le moteur pas-a-pas  -> stepper_isr_step()
 *       (2) a cadencer l'enveloppe du buzzer.
 *
 * C'est pourquoi initBeepeur() remplace son_init() dans main.c et doit etre
 * appele APRES stepper_init() (qui ne configure que les GPIO du moteur).
 * ========================================================================== */

#define GPIO_BEEP        LPC_GPIO2
#define GPIO_BEEP_NUM    4            /* P2.4 -> grille MOSFET -> buzzer actif */
#define BEEP_BIT         (1u << GPIO_BEEP_NUM)

#define TICK_US          1000u        /* tick de base = 1 ms (1 pas moteur / tick).
                                         Baisser vers 700-800 si on veut un tambour
                                         plus rapide ET que le moteur ne saute pas. */

#define BEEP_ON_MIN_MS   40           /* duree audible mini d'un bip   */
#define BEEP_ON_MAX_MS   150          /* duree audible maxi d'un bip   */

/* Etat de l'enveloppe (lu par l'ISR, ecrit par le main loop -> volatile) */
enum { BEEP_IDLE = 0, BEEP_PERIODIC = 1, BEEP_FINITE = 2 };

static volatile int beep_state  = BEEP_IDLE;
static volatile int period_ms   = 1000;   /* periode totale d'un cycle bip+silence */
static volatile int on_ms       = 120;    /* part audible dans le cycle             */
static volatile int phase_ms    = 0;      /* compteur dans le cycle courant         */
static volatile int beeps_left  = 0;      /* mode FINITE : nb de bips restants       */

/* --------- helpers ---------------------------------------------------------- */

static inline void buzzer_on(void)  { GPIO_BEEP->FIOSET = BEEP_BIT; }
static inline void buzzer_off(void) { GPIO_BEEP->FIOCLR = BEEP_BIT; }

/* Fixe periode + duree audible (clampees) a partir d'une periode en ms. */
static void set_cadence(int msecVal) {
    if (msecVal < 1) msecVal = 1;
    int on = msecVal / 2;
    if (on < BEEP_ON_MIN_MS) on = BEEP_ON_MIN_MS;
    if (on > BEEP_ON_MAX_MS) on = BEEP_ON_MAX_MS;
    if (on >= msecVal) on = msecVal;     /* periode tres courte -> quasi continu */
    period_ms = msecVal;
    on_ms     = on;
}

/* --------- API publique (signatures inchangees) ----------------------------- */

void initBeepeur(void) {
    /* P2.4 en GPIO sortie, a 0 (silence) */
    LPC_PINCON->PINSEL4 &= ~(3u << 8);
    GPIO_BEEP->FIODIR |= BEEP_BIT;
    buzzer_off();

    /* TIM1 : tick periodique de TICK_US, IRQ + reset auto sur MR0 */
    LPC_SC->PCONP    |= (1u << 2);            /* alim TIM1 (PCTIM1) */
    LPC_SC->PCLKSEL0 &= ~(3u << 4);           /* PCLK_TIM1 = CCLK/4 */
    LPC_TIM1->TCR = 0x02;                      /* arret + reset */
    {
        uint32_t pclk = SystemCoreClock / 4u; /* car PCLKSEL = CCLK/4 */
        LPC_TIM1->PR = (pclk / 1000000u) - 1u; /* -> 1 us / tick, quel que soit CCLK */
    }
    LPC_TIM1->MR0 = TICK_US;                   /* periode du tick de base */
    LPC_TIM1->MCR = (1u << 0) | (1u << 1);     /* IRQ (MR0I) + reset (MR0R) sur MR0 */
    LPC_TIM1->IR  = 0x3F;                       /* efface les flags */
    LPC_TIM1->TCR = 0x01;                       /* demarre */
    NVIC_EnableIRQ(TIMER1_IRQn);
}

/* Demarre un bip periodique. msecVal = periode totale en ms. */
void startBeep(int msecVal) {
    NVIC_DisableIRQ(TIMER1_IRQn);
    set_cadence(msecVal);
    phase_ms   = 0;
    beep_state = BEEP_PERIODIC;
    buzzer_on();                  /* premier bip immediat */
    NVIC_EnableIRQ(TIMER1_IRQn);
}

/* Met a jour la cadence pendant un bip periodique (hysteresis 200 ms anti-clignotement). */
void changeDist(int msecVal) {
    if (msecVal < 1) msecVal = 1;
    int diff = period_ms - msecVal;
    if (diff > 200 || diff < -200) {
        NVIC_DisableIRQ(TIMER1_IRQn);
        set_cadence(msecVal);
        NVIC_EnableIRQ(TIMER1_IRQn);
    }
}

/* Coupe le son. */
void stopBeep(void) {
    NVIC_DisableIRQ(TIMER1_IRQn);
    beep_state = BEEP_IDLE;
    buzzer_off();
    NVIC_EnableIRQ(TIMER1_IRQn);
}

/* Joue nbFois bips a cadence fixe (feedback charge/decharge) puis s'arrete. */
void startBeepChargeDecharge(int nbFois) {
    if (nbFois <= 0) { stopBeep(); return; }
    NVIC_DisableIRQ(TIMER1_IRQn);
    set_cadence(250);             /* cadence fixe ~4 Hz */
    phase_ms   = 0;
    beeps_left = nbFois;
    beep_state = BEEP_FINITE;
    buzzer_on();
    NVIC_EnableIRQ(TIMER1_IRQn);
}

/* --------- ISR partagee TIM1 (son + moteur pas-a-pas) ----------------------- */

void TIMER1_IRQHandler(void) {
    if (LPC_TIM1->IR & (1u << 0)) {        /* tick de base (MR0) toutes les TICK_US */
        LPC_TIM1->IR = (1u << 0);          /* acquittement (write-1-to-clear) */

        /* 1) moteur pas-a-pas : ne fait rien si aucun pas en attente */
        stepper_isr_step();

        /* 2) enveloppe du buzzer (compte en ms car TICK_US = 1000) */
        if (beep_state != BEEP_IDLE) {
            phase_ms++;

            if (phase_ms == on_ms) {
                buzzer_off();               /* fin de la partie audible du bip */
            }

            if (phase_ms >= period_ms) {
                phase_ms = 0;
                if (beep_state == BEEP_FINITE) {
                    if (beeps_left > 0) beeps_left--;
                    if (beeps_left == 0) {
                        beep_state = BEEP_IDLE;
                        buzzer_off();
                        return;
                    }
                }
                buzzer_on();                /* debut du bip suivant */
            }
        }
    }
}

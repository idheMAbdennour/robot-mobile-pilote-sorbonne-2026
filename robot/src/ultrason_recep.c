/**
 * @file ultrason_recep.c
 * @brief Décodage de l'enveloppe ultrason des postes (LPC1769, GR10).
 *
 * Protocole (miroir de EmetteurUltrason.c) :
 *   P1 (us) : 200 -> 'G', 300 -> 'D'
 *   ESPACE (us) : 500 + numPoste*200
 *   P2 : confirmation (même largeur que P1)
 *
 * ARCHITECTURE : le COEUR de décodage (machine d'état + arithmétique) est pur
 * et indépendant du matériel -> testable sur PC (voir test_ultrason.c, compilé
 * avec -DHOST_TEST). Seuls init_ultrason_recep() et l'ISR touchent les
 * registres LPC ; ils sont exclus de la compilation hôte.
 */

#include "ultrason_recep.h"

#ifndef HOST_TEST
#include "LPC17xx.h"
#include "timers.h"   /* timer2_get_tc() : compteur libre 10 us/tick */
#endif

/* ==========================================================================
 * 1) CHOIX DU PIN  (LPC176x : OBLIGATOIREMENT P0 ou P2 pour l'interruption)
 * ==========================================================================
 * Par défaut P2.13. Pour changer de pin :
 *   - même port : modifier seulement ULTRASON_RECEP_PIN
 *   - changer de port : ULTRASON_RECEP_PORT2 = 1 (P2) ou 0 (P0)
 * (P1.21 impossible : le port P1 ne fait pas d'interruption GPIO.)
 */
#define ULTRASON_RECEP_PORT2   1            /* 1 = Port2, 0 = Port0          */
#define ULTRASON_RECEP_PIN     13           /* numéro du pin (ici P2.13)     */
#define ULTRASON_RECEP_MASK    (1u << ULTRASON_RECEP_PIN)

/* ==========================================================================
 * 2) PARAMÈTRES DE DÉCODAGE  (us)
 * ========================================================================== */
#define US_TICK_US             10u          /* doit coller à TIMER2_TICK_US  */

#define US_PULSE_SEUIL_US      250u         /* < seuil => 'G' , sinon => 'D' */
#define US_PULSE_MIN_US        120u         /* fenêtre de validité impulsion */
#define US_PULSE_MAX_US        420u
#define US_PULSE_TOL_US         80u         /* écart max P1 vs P2 : < 100us (gap G/D) pour détecter un mismatch de côté */

#define US_SPACE_BASE_US       500u         /* espace pour le poste 0        */
#define US_SPACE_STEP_US       200u         /* +200 us par numéro de poste   */
#define US_SPACE_MIN_US        380u
#define US_SPACE_MAX_US        3200u        /* ~ postes 0..13                */

#define US_FRAME_TIMEOUT_US    10000u       /* réarmement si trame figée (~2.5x la trame max ~3.8ms) */

/* ==========================================================================
 * 3) ÉTAT INTERNE
 * ========================================================================== */
typedef enum { ST_IDLE, ST_P1, ST_SPACE, ST_P2 } us_rx_state_t;

static us_rx_state_t st = ST_IDLE;
static uint32_t t_r1 = 0, t_f1 = 0, t_r2 = 0;  /* timestamps (ticks 10us)    */
static uint32_t t_last = 0;                    /* dernier front vu           */

static uint8_t  res_poste = 0;
static char     res_cote  = '?';
static volatile uint8_t res_nouveau = 0;       /* drapeau "nouvelle trame"   */

/* ==========================================================================
 * 4) OUTILS (purs)
 * ========================================================================== */

/* Durée en us entre deux timestamps Timer2, robuste au débordement 32 bits. */
static uint32_t duree_us(uint32_t t0, uint32_t t1)
{
    uint32_t ticks = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFu - t0) + t1 + 1u;
    return ticks * US_TICK_US;
}

/* Tente le décodage final à partir des 3 durées mesurées. */
static void decoder(uint32_t p1_us, uint32_t space_us, uint32_t p2_us)
{
    /* validité des impulsions */
    if (p1_us < US_PULSE_MIN_US || p1_us > US_PULSE_MAX_US) return;
    if (p2_us < US_PULSE_MIN_US || p2_us > US_PULSE_MAX_US) return;

    /* P1 et P2 doivent coder le même côté (cohérence) */
    uint32_t ecart = (p1_us > p2_us) ? (p1_us - p2_us) : (p2_us - p1_us);
    if (ecart > US_PULSE_TOL_US) return;

    /* validité de l'espace */
    if (space_us < US_SPACE_MIN_US || space_us > US_SPACE_MAX_US) return;

    /* côté */
    char cote = (p1_us < US_PULSE_SEUIL_US) ? 'G' : 'D';

    /* numéro de poste : inverse de space = 500 + n*200, avec arrondi */
    uint32_t base = (space_us > US_SPACE_BASE_US) ? (space_us - US_SPACE_BASE_US) : 0u;
    uint8_t  num  = (uint8_t)((base + US_SPACE_STEP_US / 2u) / US_SPACE_STEP_US);

    res_poste   = num;
    res_cote    = cote;
    res_nouveau = 1;
}

/* ==========================================================================
 * 5) COEUR DE DÉCODAGE (pur, testable)
 * ========================================================================== */

/* Traite un front : niveau=1 (montant), niveau=0 (descendant), now en ticks. */
void ultrason_decode_edge(uint8_t niveau, uint32_t now_ticks)
{
    t_last = now_ticks;

    if (niveau) {                       /* ---- FRONT MONTANT ---- */
        if (st == ST_SPACE) {
            t_r2 = now_ticks;           /* début de P2 */
            st   = ST_P2;
        } else {
            t_r1 = now_ticks;           /* (re)synchro : début de P1 */
            st   = ST_P1;
        }
    } else {                            /* ---- FRONT DESCENDANT ---- */
        if (st == ST_P1) {
            t_f1 = now_ticks;           /* fin de P1 -> espace */
            st   = ST_SPACE;
        } else if (st == ST_P2) {
            uint32_t p1_us    = duree_us(t_r1, t_f1);
            uint32_t space_us = duree_us(t_f1, t_r2);
            uint32_t p2_us    = duree_us(t_r2, now_ticks);
            decoder(p1_us, space_us, p2_us);
            st = ST_IDLE;
        } else {
            st = ST_IDLE;               /* front incohérent : on réarme */
        }
    }
}

/* Réarme si une trame partielle reste figée trop longtemps. */
void ultrason_decode_timeout(uint32_t now_ticks)
{
    if (st != ST_IDLE) {
        if (duree_us(t_last, now_ticks) > US_FRAME_TIMEOUT_US) {
            st = ST_IDLE;
        }
    }
}

/* ==========================================================================
 * 6) MATÉRIEL (exclu de la compilation hôte)
 * ========================================================================== */
#ifndef HOST_TEST

void init_ultrason_recep(void)
{
#if ULTRASON_RECEP_PORT2
    /* P2.x en GPIO : PINSEL4 bits [2*PIN+1 : 2*PIN] = 00 */
    LPC_PINCON->PINSEL4 &= ~(3u << (ULTRASON_RECEP_PIN * 2));
    LPC_GPIO2->FIODIR   &= ~ULTRASON_RECEP_MASK;     /* entrée */
    LPC_GPIOINT->IO2IntEnR |= ULTRASON_RECEP_MASK;
    LPC_GPIOINT->IO2IntEnF |= ULTRASON_RECEP_MASK;
    LPC_GPIOINT->IO2IntClr  = ULTRASON_RECEP_MASK;
#else
    LPC_PINCON->PINSEL0 &= ~(3u << (ULTRASON_RECEP_PIN * 2)); /* P0.0..15 */
    LPC_GPIO0->FIODIR   &= ~ULTRASON_RECEP_MASK;
    LPC_GPIOINT->IO0IntEnR |= ULTRASON_RECEP_MASK;
    LPC_GPIOINT->IO0IntEnF |= ULTRASON_RECEP_MASK;
    LPC_GPIOINT->IO0IntClr  = ULTRASON_RECEP_MASK;
#endif

    NVIC_EnableIRQ(EINT3_IRQn);   /* idempotent (déjà activé par d'autres) */

    st = ST_IDLE;
    res_nouveau = 0;
}

void ultrason_recep_interrupt_routine(void)
{
#if ULTRASON_RECEP_PORT2
    uint32_t st_rise = LPC_GPIOINT->IO2IntStatR & ULTRASON_RECEP_MASK;
    uint32_t st_fall = LPC_GPIOINT->IO2IntStatF & ULTRASON_RECEP_MASK;
#else
    uint32_t st_rise = LPC_GPIOINT->IO0IntStatR & ULTRASON_RECEP_MASK;
    uint32_t st_fall = LPC_GPIOINT->IO0IntStatF & ULTRASON_RECEP_MASK;
#endif

    if (!st_rise && !st_fall) return;   /* pas notre pin */

    uint32_t now = timer2_get_tc();
    if (st_rise) ultrason_decode_edge(1u, now);
    if (st_fall) ultrason_decode_edge(0u, now);

#if ULTRASON_RECEP_PORT2
    LPC_GPIOINT->IO2IntClr = ULTRASON_RECEP_MASK;
#else
    LPC_GPIOINT->IO0IntClr = ULTRASON_RECEP_MASK;
#endif
}

void ultrason_recep_tick(void)
{
    ultrason_decode_timeout(timer2_get_tc());
}

#endif /* !HOST_TEST */

/* ==========================================================================
 * 7) LECTURE DU RÉSULTAT (pure)
 * ========================================================================== */
uint8_t ultrason_recep_lire(uint8_t *num_poste, char *cote)
{
    if (!res_nouveau) return 0;
    if (num_poste) *num_poste = res_poste;
    if (cote)      *cote      = res_cote;
    res_nouveau = 0;
    return 1;
}
/*======================================================================
 *  proximetre.c  —  LPC1769
 *  Détection d'obstacles : servo (balayage) + GP2Y0A02YK0F (ADC)
 *                           + UART de débogage + buzzer
 *
 *  Modules : uart.h, robotState.h, proximetre.h
 *  Les vecteurs d'interruption se trouvent dans interruptions.c — le module
 *  fournit seulement les routines :
 *    - proximetre_interrupt_routine()        <- depuis EINT3_IRQHandler (switchs)
 *    - proximetre_timer_interrupt_routine()  <- depuis TIMER3_IRQHandler (servo)
 *
 *  Horloges : CCLK=100MHz, PCLK=CCLK/4=25MHz.
 *
 *  >>> PWM / TEMPORISATEURS <<<
 *  PWM1 est utilisé par les moteurs (25 kHz, période commune) — le servo
 *  ne peut pas y être ajouté.
 *  Timer0 (emissionIR) et Timer1 sont utilisés. Tout le proximètre fonctionne
 *  avec Timer3 :
 *    - servo : PWM logiciel par interruption (MR0=front↑, MR1=front↓)
 *    - pauses et buzzer : lecture du TC en libre comptage (1 µs)
 *
 *  Si Timer3 n'est PAS libre, modifier ici : LPC_TIM3->LPC_TIM2,
 *  PCONP (1<<23)->(1<<22), TIMER3_IRQn->TIMER2_IRQn ; et dans interruptions.c
 *  renommer TIMER3_IRQHandler -> TIMER2_IRQHandler.
 *
 *  PLAN DES BROCHES :
 *    Servo sig : P2.2  -> GPIO + ISR Timer3  (J2-43)
 *    IR Vo ADC : P0.23 -> AD0.0    (J2-15)
 *    UART0 dbg : P0.2 TXD0 / P0.3 RXD0
 *    Buzzer    : P0.21 -> sortie GPIO (J2-23)
 *    Switch S0 : P0.4  -> entrée GPIO  ] 2 micro-switchs -> 4 modes
 *    Switch S1 : P0.5  -> entrée GPIO
 *====================================================================*/

#include "LPC17xx.h"
#include "proximetre.h"
#include "robotState.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>     /* abs */
#include <math.h>       /* powf (seulement si USE_LUT=0) */

/*---------------------------------------------------------------------
 *  BROCHES
 *-------------------------------------------------------------------*/
#define PIN_PROX_SW1   (1u << 12)    /* P0.12 */
#define PIN_PROX_SW2   (1u << 13)    /* P0.13 */
#define SERVO_PIN      (1u << 2)    /* P2.2 (GPIO2) — signal du servo */
#define BUZZ_PIN       (1u << 21)   /* P0.21 (GPIO0) */

/*---------------------------------------------------------------------
 *  PARAMÈTRES (à calibrer / vérifier avec le cahier des charges — marqué !!!)
 *-------------------------------------------------------------------*/
#define ADC_VREF        3.3f
#define ADC_MAX         4095.0f
#define ADC_AVG_N       8

#define STEP_DEG        5
#define DIST_MIN_CM     20
#define DIST_MAX_CM     150

#define US_PER_DEG      11
#define SERVO_CENTER_US 1500
#define SERVO_PERIOD_US 20000u      /* 20 ms */

#define BUZZ_ON_US      250000u     /* bip 0,25 s (cahier des charges p.10) */

#define USE_LUT         1

/*---------------------------------------------------------------------
 *  Modes de balayage (structure du cahier des charges p.9 ; vérifier les nombres)
 *-------------------------------------------------------------------*/
typedef struct { int angle_max; uint32_t step_delay_ms; } mode_t;
static const mode_t MODES[4] = {
    /* 00 */ { 60, 40 },
    /* 01 */ { 30, 38 },
    /* 10 */ { 20, 37 },
    /* 11 */ { 15, 36 }
};

/* Linéarisation du GP2Y0A02YK0F (V avec l'objet, d'après la fiche technique) */
static const float LUT_V[]  = {2.5f,2.0f,1.55f,1.25f,1.1f,0.85f,0.8f,
                               0.73f,0.7f,0.65f,0.6f,0.5f,0.45f,0.4f};
static const int   LUT_CM[] = { 20, 30, 40, 50, 60, 70, 80,
                                90,100,110,120,130,140,150};
#define LUT_N (sizeof(LUT_CM)/sizeof(LUT_CM[0]))

/*---------------------------------------------------------------------
 *  État du module
 *-------------------------------------------------------------------*/
static volatile uint8_t  prox_mode = 0;          /* écrit dans l'ISR des switchs */
static int32_t  prox_buf[NUM_PROXI_MEASUREMENTS];
static int      prox_count = 0;
static char     prox_dir = 'T';

static volatile uint32_t servo_pulse_us = SERVO_CENTER_US;  /* lu dans l'ISR du servo */

static volatile uint32_t buzz_period_us = 0;
static uint32_t buzz_t0 = 0;
static uint8_t  buzz_on = 0;

/* déclarations anticipées */
static void init_proximetre_switches(void);
static void proximetre_update_mode_from_gpio(void);
static void Timer3_Init(void);
static void Servo_SetAngle(int angle_deg);
static void ADC_Init(void);
static uint16_t ADC_ReadAvg(void);
static int  Distance_cm(uint16_t adc);
static void Buzzer_Init(void);
static void Buzzer_SetFromDistance(int cm);
static void Buzzer_Service(void);
static void delay_ms(uint32_t ms);

/*=====================================================================
 *  Timer3 — comptage libre à 1 µs. Gère le servo (ISR) + la base de temps (TC).
 *===================================================================*/
static void Timer3_Init(void)
{
    LPC_PINCON->PINSEL4 &= ~(3u << 4);          /* P2.2 = GPIO */
    LPC_GPIO2->FIODIR   |=  SERVO_PIN;
    LPC_GPIO2->FIOCLR    =  SERVO_PIN;

    LPC_SC->PCONP |= (1u << 23);                /* alimentation de Timer3 */
    LPC_TIM3->TCR  = 0x02;                      /* réinitialisation, TC=0 */
    LPC_TIM3->PR   = 24;                        /* 25MHz/(24+1)=1MHz -> 1 µs */
    LPC_TIM3->MCR  = (1u << 0) | (1u << 3);     /* IRQ sur MR0 et MR1, SANS réinitialisation */
    LPC_TIM3->MR0  = 1000u;                     /* premier front↑ dans 1 ms */
    LPC_TIM3->MR1  = 1000u + SERVO_CENTER_US;   /* premier front↓ */
    LPC_TIM3->IR   = 0x3F;                      /* effacer les indicateurs */
    NVIC_EnableIRQ(TIMER3_IRQn);
    LPC_TIM3->TCR  = 0x01;                      /* démarrage (comptage libre) */
}

/* Appelée depuis TIMER3_IRQHandler (définie dans interruptions.c) */
void proximetre_timer_interrupt_routine(void)
{
    uint32_t ir = LPC_TIM3->IR;

    if (ir & (1u << 0)) {                       /* MR0 : front montant */
        LPC_GPIO2->FIOSET = SERVO_PIN;
        uint32_t base = LPC_TIM3->MR0;          /* grille idéale, sans dérive */
        LPC_TIM3->MR1 = base + servo_pulse_us;  /* moment où abaisser */
        LPC_TIM3->MR0 = base + SERVO_PERIOD_US; /* période suivante */
        LPC_TIM3->IR  = (1u << 0);
    }
    if (ir & (1u << 1)) {                       /* MR1 : front descendant */
        LPC_GPIO2->FIOCLR = SERVO_PIN;
        LPC_TIM3->IR  = (1u << 1);
    }
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = LPC_TIM3->TC;
    while ((uint32_t)(LPC_TIM3->TC - start) < ms * 1000u) {
        Buzzer_Service();
    }
}

static void Servo_SetAngle(int angle_deg)
{
    int pulse = SERVO_CENTER_US + angle_deg * US_PER_DEG;
    if (pulse < 700)  pulse = 700;
    if (pulse > 2300) pulse = 2300;
    servo_pulse_us = (uint32_t)pulse;           /* l'ISR le prendra en compte à partir de la période suivante */
}

/*=====================================================================
 *  ADC — AD0.0 sur P0.23
 *===================================================================*/
static void ADC_Init(void)
{
    LPC_SC->PCONP    |= (1u << 12);
    LPC_PINCON->PINSEL1 &= ~(3u << 14);
    LPC_PINCON->PINSEL1 |=  (1u << 14);          /* P0.23 = AD0.0 */
    LPC_ADC->ADCR = (1u << 0) | (4u << 8) | (1u << 21);
}

static uint16_t ADC_ReadCh0(void)
{
    LPC_ADC->ADCR &= ~(7u << 24);
    LPC_ADC->ADCR |=  (1u << 24);
    while (!(LPC_ADC->ADGDR & (1u << 31)));
    return (uint16_t)((LPC_ADC->ADGDR >> 4) & 0x0FFF);
}

static uint16_t ADC_ReadAvg(void)
{
    uint32_t s = 0;
    for (int i = 0; i < ADC_AVG_N; i++) s += ADC_ReadCh0();
    return (uint16_t)(s / ADC_AVG_N);
}

/*=====================================================================
 *  ADC -> centimètres
 *===================================================================*/
static int Distance_cm(uint16_t adc)
{
    float v = (adc / ADC_MAX) * ADC_VREF;
#if USE_LUT
    if (v >= LUT_V[0])        return DIST_MIN_CM;
    if (v <= LUT_V[LUT_N-1])  return DIST_MAX_CM;
    for (uint32_t i = 1; i < LUT_N; i++) {
        if (v >= LUT_V[i]) {
            float t = (v - LUT_V[i]) / (LUT_V[i-1] - LUT_V[i]);
            return (int)(LUT_CM[i] + t * (LUT_CM[i-1] - LUT_CM[i]) + 0.5f);
        }
    }
    return DIST_MAX_CM;
#else
    if (v < 0.05f) return DIST_MAX_CM;
    int cm = (int)(65.0f * powf(v, -1.10f) + 0.5f);
    if (cm < DIST_MIN_CM) cm = DIST_MIN_CM;
    if (cm > DIST_MAX_CM) cm = DIST_MAX_CM;
    return cm;
#endif
}

/*=====================================================================
 *  Buzzer (P0.21) — fréquence de répétition du bip (cahier des charges p.10)
 *===================================================================*/
static void Buzzer_Init(void)
{
    LPC_PINCON->PINSEL1 &= ~(3u << 10);          /* P0.21 = GPIO */
    LPC_GPIO0->FIODIR   |=  BUZZ_PIN;
    LPC_GPIO0->FIOCLR    =  BUZZ_PIN;
}

static void Buzzer_SetFromDistance(int cm)
{
    if (cm < DIST_MIN_CM || cm > 100) { buzz_period_us = 0; return; }
    float f = 0.5f + (100 - cm) * 0.025f;
    buzz_period_us = (uint32_t)(1000000.0f / f + 0.5f);
}

static void Buzzer_Service(void)
{
    if (buzz_period_us == 0) {
        if (buzz_on) { LPC_GPIO0->FIOCLR = BUZZ_PIN; buzz_on = 0; }
        return;
    }
    uint32_t off = (buzz_period_us > BUZZ_ON_US) ? (buzz_period_us - BUZZ_ON_US) : 1000u;
    uint32_t dur = buzz_on ? BUZZ_ON_US : off;
    if ((uint32_t)(LPC_TIM3->TC - buzz_t0) >= dur) {
        buzz_on = !buzz_on;
        buzz_t0 = LPC_TIM3->TC;
        if (buzz_on) LPC_GPIO0->FIOSET = BUZZ_PIN;
        else         LPC_GPIO0->FIOCLR = BUZZ_PIN;
    }
}

/*=====================================================================
 *  Switchs de mode (P0.4, P0.5)
 *===================================================================*/
static void init_proximetre_switches(void)
{
    LPC_PINCON->PINSEL0 &= ~((3u << 8) | (3u << 10)); /* P0.4,P0.5 = GPIO */
    LPC_GPIO0->FIODIR   &= ~(PIN_PROX_SW1 | PIN_PROX_SW2);
    LPC_GPIOINT->IO0IntEnR |= (PIN_PROX_SW1 | PIN_PROX_SW2);
    LPC_GPIOINT->IO0IntEnF |= (PIN_PROX_SW1 | PIN_PROX_SW2);
}

static void proximetre_update_mode_from_gpio(void)
{
    uint8_t sw1 = (LPC_GPIO0->FIOPIN & PIN_PROX_SW1) ? 1u : 0u;
    uint8_t sw2 = (LPC_GPIO0->FIOPIN & PIN_PROX_SW2) ? 1u : 0u;
    prox_mode = (uint8_t)((sw2 << 1) | sw1);
}

void init_proximetre(void)
{
    Timer3_Init();
    ADC_Init();
    Buzzer_Init();
    init_proximetre_switches();
    proximetre_update_mode_from_gpio();
    Servo_SetAngle(0);
    delay_ms(500);
    NVIC_EnableIRQ(EINT3_IRQn);          /* les interruptions GPIO passent par EINT3 */
}

void proximetre_interrupt_routine(void)  /* appelée depuis l'EINT3_IRQHandler commun */
{
    if ((LPC_GPIOINT->IO0IntStatR & (PIN_PROX_SW1 | PIN_PROX_SW2)) ||
        (LPC_GPIOINT->IO0IntStatF & (PIN_PROX_SW1 | PIN_PROX_SW2)))
    {
        proximetre_update_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_PROX_SW1 | PIN_PROX_SW2);
    }
}

/*=====================================================================
 *  Routine de balayage : un passage, alterne avant/arrière
 *  ATTENTION : bloquante (delay_ms). Avec un main coopératif à 50 Hz, cela
 *  bloquera les autres tâches pendant la durée du passage. Voir la remarque
 *  dans le chat — pour ton architecture, il vaut mieux la transformer en
 *  automate tick non bloquant.
 *===================================================================*/
void proximetre_run_balayage(void)
{
    static int going_fwd = 1;
    mode_t m   = MODES[prox_mode & 0x3];
    int amax   = m.angle_max;
    int n      = (2 * amax) / STEP_DEG + 1;
    if (n > NUM_PROXI_MEASUREMENTS) n = NUM_PROXI_MEASUREMENTS;

    int i = 0, front = DIST_MAX_CM;

    if (going_fwd) {
        for (int a = -amax; a <= amax && i < n; a += STEP_DEG) {
            Servo_SetAngle(a);
            delay_ms(m.step_delay_ms);
            int cm = Distance_cm(ADC_ReadAvg());
            prox_buf[i++] = cm;
            if (abs(a) <= 10 && cm < front) front = cm;
        }
        prox_dir = 'T';
    } else {
        for (int a = amax; a >= -amax && i < n; a -= STEP_DEG) {
            Servo_SetAngle(a);
            delay_ms(m.step_delay_ms);
            int cm = Distance_cm(ADC_ReadAvg());
            prox_buf[i++] = cm;
            if (abs(a) <= 10 && cm < front) front = cm;
        }
        prox_dir = 't';
    }
    prox_count = i;
    going_fwd  = !going_fwd;

    Buzzer_SetFromDistance(front);

    /* TODO robotState : en cas d'obstacle, définir un indicateur pour le reste de la logique,
       par exemple : robot_set_obstacle(front < SEUIL_CM);  (adapter l'API) */
}

/*=====================================================================
 *  Accesseurs + trame de débogage
 *===================================================================*/
void get_proxi_distances(int32_t *out)
{
    for (int i = 0; i < prox_count; i++) out[i] = prox_buf[i];
}

int get_proxi_count(void) { return prox_count; }

void debug_proximetre_send_frame(void)
{
    char buffer[160];
    int  offset = 0;
    int32_t mesures[NUM_PROXI_MEASUREMENTS];
    int  cnt = get_proxi_count();

    get_proxi_distances(mesures);

    offset += sprintf(buffer + offset, "%c", prox_dir);
    for (int i = 0; i < cnt; i++)
        offset += sprintf(buffer + offset, " %03d", (int)mesures[i]);
    sprintf(buffer + offset, "\r\n");

    uart0_send_string(buffer);
}
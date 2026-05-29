#include "LPC17xx.h"
#include "proximetre.h"
#include "robotState.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>     
#include <math.h>       

#define SERVO_PIN      (1u << 2)    
#define BUZZ_PIN       (1u << 21)   

#define ADC_VREF        3.3f
#define ADC_MAX         4095.0f
#define ADC_AVG_N       8

#define STEP_DEG        5
#define DIST_MIN_CM     20
#define DIST_MAX_CM     150

#define US_PER_DEG      11
#define SERVO_CENTER_US 1500
#define SERVO_PERIOD_US 20000u      

#define BUZZ_ON_US      250000u     

#define USE_LUT         1

typedef struct { int angle_max; uint32_t step_delay_ms; } mode_t;
static const mode_t MODES[4] = {
    { 60, 40 },
    { 30, 38 },
    { 20, 37 },
    { 15, 36 }
};

static const float LUT_V[]  = {2.5f,2.0f,1.55f,1.25f,1.1f,0.85f,0.8f,
                               0.73f,0.7f,0.65f,0.6f,0.5f,0.45f,0.4f};
static const int   LUT_CM[] = { 20, 30, 40, 50, 60, 70, 80,
                                90,100,110,120,130,140,150};
#define LUT_N (sizeof(LUT_CM)/sizeof(LUT_CM[0]))

static volatile uint8_t  prox_mode = 0;          
static int32_t  prox_buf[NUM_PROXI_MEASUREMENTS];
static int      prox_count = 0;
static char     prox_dir = 'T';

static volatile uint32_t servo_pulse_us = SERVO_CENTER_US;  

static volatile uint32_t buzz_period_us = 0;
static uint32_t buzz_t0 = 0;
static uint8_t  buzz_on = 0;

/* Внутренние прототипы */
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

static void Timer3_Init(void)
{
    LPC_PINCON->PINSEL4 &= ~(3u << 4);          
    LPC_GPIO2->FIODIR   |=  SERVO_PIN;
    LPC_GPIO2->FIOCLR    =  SERVO_PIN;

    LPC_SC->PCONP |= (1u << 23);                
    LPC_TIM3->TCR  = 0x02;                      
    LPC_TIM3->PR   = 24;                        
    LPC_TIM3->MCR  = (1u << 0) | (1u << 3);     
    LPC_TIM3->MR0  = 1000u;                     
    LPC_TIM3->MR1  = 1000u + SERVO_CENTER_US;   
    LPC_TIM3->IR   = 0x3F;                      
    NVIC_EnableIRQ(TIMER3_IRQn);
    LPC_TIM3->TCR  = 0x01;                      
}

void proximetre_timer_interrupt_routine(void)
{
    uint32_t ir = LPC_TIM3->IR;

    if (ir & (1u << 0)) {                       
        LPC_GPIO2->FIOSET = SERVO_PIN;
        uint32_t base = LPC_TIM3->MR0;          
        LPC_TIM3->MR1 = base + servo_pulse_us;  
        LPC_TIM3->MR0 = base + SERVO_PERIOD_US; 
        LPC_TIM3->IR  = (1u << 0);
    }
    if (ir & (1u << 1)) {                       
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
    servo_pulse_us = (uint32_t)pulse;           
}

static void ADC_Init(void)
{
    LPC_SC->PCONP    |= (1u << 12);
    LPC_PINCON->PINSEL1 &= ~(3u << 14);
    LPC_PINCON->PINSEL1 |=  (1u << 14);          
    LPC_ADC->ADCR = (1u << 0) | (4u << 8) | (1u << 21);
}

static uint16_t ADC_ReadCh0(void)
{
    NVIC_DisableIRQ(EINT3_IRQn);        
    LPC_ADC->ADCR &= ~(0xFFu);          
    LPC_ADC->ADCR |=  (1u << 0);        
    LPC_ADC->ADCR &= ~(7u << 24);
    LPC_ADC->ADCR |=  (1u << 24);      
    while (!(LPC_ADC->ADGDR & (1u << 31)));
    uint16_t v = (LPC_ADC->ADGDR >> 4) & 0x0FFF;
    NVIC_EnableIRQ(EINT3_IRQn);
    return v;
}

static uint16_t ADC_ReadAvg(void)
{
    uint32_t s = 0;
    for (int i = 0; i < ADC_AVG_N; i++) s += ADC_ReadCh0();
    return (uint16_t)(s / ADC_AVG_N);
}

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

static void Buzzer_Init(void)
{
    LPC_PINCON->PINSEL1 &= ~(3u << 10);          
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

static void init_proximetre_switches(void)
{
    LPC_PINCON->PINSEL0 &= ~((3u << 8) | (3u << 10)); 
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
    NVIC_EnableIRQ(EINT3_IRQn);          
}

void proximetre_interrupt_routine(void)  
{
    if ((LPC_GPIOINT->IO0IntStatR & (PIN_PROX_SW1 | PIN_PROX_SW2)) ||
        (LPC_GPIOINT->IO0IntStatF & (PIN_PROX_SW1 | PIN_PROX_SW2)))
    {
        proximetre_update_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_PROX_SW1 | PIN_PROX_SW2);
    }
}

/* --- Balayage non bloquant : machine à états cadencée à 50 Hz --- */
static int      bal_in_progress = 0;   /* balayage en cours ? */
static int      bal_going_fwd   = 1;   /* sens courant (T = aller, t = retour) */
static int      bal_angle;             /* angle cible courant */
static int      bal_amax;              /* amplitude du mode courant */
static int      bal_n;                 /* nb de mesures prévues ce balayage */
static int      bal_i;                 /* index courant dans prox_buf */
static int      bal_front;             /* min distance frontale (-10..+10°) */
static uint32_t bal_step_us;           /* délai de stabilisation (µs) du mode */
static uint32_t bal_t0;                /* horodatage TC du dernier Servo_SetAngle */

static void prox_begin_sweep(void)
{
    mode_t m    = MODES[prox_mode & 0x3];
    bal_amax    = m.angle_max;
    bal_step_us = m.step_delay_ms * 1000u;
    bal_n       = (2 * bal_amax) / STEP_DEG + 1;
    if (bal_n > NUM_PROXI_MEASUREMENTS) bal_n = NUM_PROXI_MEASUREMENTS;
    bal_i     = 0;
    bal_front = DIST_MAX_CM;
    bal_angle = bal_going_fwd ? -bal_amax : bal_amax;
    Servo_SetAngle(bal_angle);
    bal_t0          = LPC_TIM3->TC;
    bal_in_progress = 1;
}

static void prox_finish_sweep(void)
{
    prox_count = bal_i;
    prox_dir   = bal_going_fwd ? 'T' : 't';

    Buzzer_SetFromDistance(bal_front);
    if (bal_front < 30) {
        set_robot_status(STATUS_RDV_EXPEDITION);
    }
    if (get_debug_uart_enabled()) {
        debug_proximetre_send_frame();   /* « à chaque balayage » */
    }

    bal_going_fwd   = !bal_going_fwd;    /* le balayage suivant repart en sens inverse */
    bal_in_progress = 0;
}

void proximetre_tick(void)
{
    Buzzer_Service();                    /* entretient le bip (remplace le service fait dans delay_ms) */

    if (!bal_in_progress) prox_begin_sweep();

    if ((uint32_t)(LPC_TIM3->TC - bal_t0) < bal_step_us) {
        return;                          /* servo pas encore stabilisé */
    }

    int cm = Distance_cm(ADC_ReadAvg());
    prox_buf[bal_i++] = cm;
    set_proxi_distance_at_angle(bal_angle, (int32_t)cm);
    if (abs(bal_angle) <= 10 && cm < bal_front) bal_front = cm;

    bal_angle += bal_going_fwd ? STEP_DEG : -STEP_DEG;
    int done = bal_going_fwd ? (bal_angle > bal_amax  || bal_i >= bal_n)
                             : (bal_angle < -bal_amax || bal_i >= bal_n);
    if (done) {
        prox_finish_sweep();
    } else {
        Servo_SetAngle(bal_angle);
        bal_t0 = LPC_TIM3->TC;           /* relance le chrono de stabilisation */
    }
}

void get_local_proxi_buffer(int32_t *out)
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

    get_local_proxi_buffer(mesures);

    offset += sprintf(buffer + offset, "%c", prox_dir);
    for (int i = 0; i < cnt; i++)
        offset += sprintf(buffer + offset, " %03d", (int)mesures[i]);
    sprintf(buffer + offset, "\r\n");

    uart0_send_string(buffer);
}
/**
 * @file proximetre.c
 * @brief Fichier du module proximetre.
 */

#include "proximetre.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "LPC17xx.h"

#include "adc.h"
#include "robot_state.h"
#include "timers.h"
#include "uart.h"

/* ==========================================================================
 * DÉFINITIONS PRIVÉES
 * ========================================================================== */
#define ADC_VREF        3.3f
#define ADC_MAX         4095.0f
#define ADC_AVG_N       8

// PROXI_STEP_DEG est maintenant défini dans proximetre.h
#define DIST_MIN_CM     20
#define DIST_MAX_CM     150

#define US_PER_DEG      11
#define SERVO_CENTER_US 1500
#define SERVO_PERIOD_US 20000u

#define BUZZ_ON_US      250000u

#define USE_LUT         1

typedef struct {
    int angle_max;
    uint32_t step_delay_ms;
} mode_t;

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

/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */
static volatile uint8_t prox_mode = 0;
static int32_t prox_buf[NUM_PROXI_MEASUREMENTS];
static int prox_count = 0;
static char prox_dir = 'T';

static volatile uint32_t servo_pulse_us = SERVO_CENTER_US;

static volatile uint32_t buzz_period_us = 0;
static uint32_t buzz_t0 = 0;
static uint8_t buzz_on = 0;

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void init_proximetre_switches(void);
static void proximetre_update_mode_from_gpio(void);
static void init_servo_hardware(void);
static void servo_set_angle(int angle_deg);
static void adc_init(void);
static uint16_t adc_read_avg(void);
static int distance_cm(uint16_t adc);
static void buzzer_init(void);
static void buzzer_set_from_distance(int cm);
static void buzzer_service(void);
static void delay_ms(uint32_t ms);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PRIVÉES
 * ========================================================================== */

static void init_servo_hardware(void) {
    LPC_PINCON->PINSEL4 &= ~(3u << 6); // P2.3 GPIO
    PORT_SERVO->FIODIR   |=  SERVO_PIN;
    PORT_SERVO->FIOCLR    =  SERVO_PIN;

    timer3_init_servo(SERVO_PERIOD_US, SERVO_CENTER_US);
}

void proximetre_timer_interrupt_routine(void) {
    uint32_t ir = LPC_TIM3->IR; // Accès matériel direct pour les interruptions très rapides

    if (ir & (1u << 0)) {
        PORT_SERVO->FIOSET = SERVO_PIN;
        uint32_t base = timer3_get_match0();
        timer3_set_match1(base + servo_pulse_us);
        timer3_set_match0(base + SERVO_PERIOD_US);
        timer3_clear_interrupt(1u << 0);
    }
    if (ir & (1u << 1)) {
        PORT_SERVO->FIOCLR = SERVO_PIN;
        timer3_clear_interrupt(1u << 1);
    }
}

static void delay_ms(uint32_t ms) {
    uint32_t start = timer3_get_tc();
    while ((uint32_t)(timer3_get_tc() - start) < ms * 1000u) {
        buzzer_service();
    }
}

static void servo_set_angle(int angle_deg) {
    int pulse = SERVO_CENTER_US + angle_deg * US_PER_DEG;
    if (pulse < 700)  pulse = 700;
    if (pulse > 2300) pulse = 2300;
    servo_pulse_us = (uint32_t)pulse;
}

static void adc_init(void) {
    adc_pin_config(PROXIMETRE_ADC_CH);
}

static uint16_t adc_read_avg(void) {
    adc_request_proximetre_avg(ADC_AVG_N);

    // Attente active très courte (~100µs).
    // Les interruptions étant actives, le capteur inductif peut
    // préempter cette attente à tout moment sans rater son signal !
    while (!adc_is_proximetre_ready());

    return adc_get_proximetre_value();
}

static int distance_cm(uint16_t adc) {
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

static void buzzer_init(void) {
    LPC_PINCON->PINSEL4 &= ~(3u << 8); // P2.4 GPIO
    PORT_BUZZ->FIODIR   |=  BUZZ_PIN;
    PORT_BUZZ->FIOCLR    =  BUZZ_PIN;
}

static void buzzer_set_from_distance(int cm) {
    if (cm < DIST_MIN_CM || cm > 100) { buzz_period_us = 0; return; }
    float f = 0.5f + (100 - cm) * 0.025f;
    buzz_period_us = (uint32_t)(1000000.0f / f + 0.5f);
}

static void buzzer_service(void) {
    if (buzz_period_us == 0) {
        if (buzz_on) { PORT_BUZZ->FIOCLR = BUZZ_PIN; buzz_on = 0; }
        return;
    }
    uint32_t off = (buzz_period_us > BUZZ_ON_US) ? (buzz_period_us - BUZZ_ON_US) : 1000u;
    uint32_t dur = buzz_on ? BUZZ_ON_US : off;
    if ((uint32_t)(timer3_get_tc() - buzz_t0) >= dur) {
        buzz_on = !buzz_on;
        buzz_t0 = timer3_get_tc();
        if (buzz_on) PORT_BUZZ->FIOSET = BUZZ_PIN;
        else         PORT_BUZZ->FIOCLR = BUZZ_PIN;
    }
}

static void init_proximetre_switches(void) {
    // Configurer P1.30 et P1.31 en GPIO (PINSEL3 bits 28:29 et 30:31)
    LPC_PINCON->PINSEL3 &= ~((3u << 28) | (3u << 30));
    LPC_PINCON->PINMODE3 &= ~((3u << 28) | (3u << 30)); // Pull-up par défaut
    PORT_PROX_SW->FIODIR   &= ~(PIN_PROX_SW1 | PIN_PROX_SW2);
}

static void proximetre_update_mode_from_gpio(void) {
    uint8_t sw1 = (PORT_PROX_SW->FIOPIN & PIN_PROX_SW1) ? 1u : 0u;
    uint8_t sw2 = (PORT_PROX_SW->FIOPIN & PIN_PROX_SW2) ? 1u : 0u;
    prox_mode = (uint8_t)((sw2 << 1) | sw1);
}

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PUBLIQUES
 * ========================================================================== */

void init_proximetre(void) {
    init_servo_hardware();
    adc_init();
    buzzer_init();
    init_proximetre_switches();
    proximetre_update_mode_from_gpio();
    servo_set_angle(0);
    delay_ms(500);
    NVIC_EnableIRQ(EINT3_IRQn);
}



void proximetre_run_balayage(void) {
    // 1. Service du buzzer (non-bloquant, appelé à 50Hz)
    buzzer_service();

    // 2. Variables de la machine d'état
    static int going_fwd = 1;
    static int step_index = 0;
    static int current_angle = 0;
    static int front_dist = DIST_MAX_CM;
    static int ms_accumulated = 0;
    static int initialized = 0;

    mode_t m = MODES[prox_mode & 0x3];
    int amax = m.angle_max;
    int n = (2 * amax) / PROXI_STEP_DEG + 1;
    if (n > NUM_PROXI_MEASUREMENTS) n = NUM_PROXI_MEASUREMENTS;

    // Initialisation au premier appel ou changement de balayage
    if (!initialized) {
        current_angle = going_fwd ? -amax : amax;
        servo_set_angle(current_angle);
        initialized = 1;
        return; // On attend le prochain tick pour lire la valeur (le temps que le servo bouge)
    }

    // Accumulation du temps (chaque appel correspond à 20ms dans une boucle à 50Hz)
    ms_accumulated += 20;
    if (ms_accumulated < (int)m.step_delay_ms) {
        return; // Le délai pour ce pas n'est pas encore écoulé
    }
    // Délai écoulé, on consomme le temps et on effectue la mesure
    ms_accumulated -= (int)m.step_delay_ms;

    // --- ÉTAPE DE MESURE ---
    // On mesure la distance pour l'angle actuel (qui a eu le temps de se stabiliser)
    int cm = distance_cm(adc_read_avg());
    prox_buf[step_index] = cm;
    set_proxi_distance_at_angle(current_angle, (int32_t)cm);

    if (abs(current_angle) <= 10 && cm < front_dist) {
        front_dist = cm;
    }

    step_index++;

    // --- ÉTAPE DE TRANSITION ---
    if (step_index >= n) {
        // Fin du balayage dans un sens
        prox_count = step_index;
        prox_dir = going_fwd ? 'T' : 't';

        // Mise à jour du buzzer et statut avec la distance frontale minimale trouvée
        buzzer_set_from_distance(front_dist);
        if (front_dist < 30) {
            set_robot_status(STATUS_RDV_EXPEDITION);
        }

        // Préparation du prochain balayage (changement de sens)
        going_fwd = !going_fwd;
        step_index = 0;
        front_dist = DIST_MAX_CM;

        // Recharger le mode périodiquement
        proximetre_update_mode_from_gpio();
        m = MODES[prox_mode & 0x3];
        amax = m.angle_max;
        current_angle = going_fwd ? -amax : amax;
    } else {
        // Préparation du pas suivant
        if (going_fwd) {
            current_angle += PROXI_STEP_DEG;
        } else {
            current_angle -= PROXI_STEP_DEG;
        }
    }

    // Commande du servomoteur pour le nouvel angle
    servo_set_angle(current_angle);
}

void get_local_proxi_buffer(int32_t *out) {
    for (int i = 0; i < prox_count; i++) out[i] = prox_buf[i];
}

int get_proxi_count(void) {
    return prox_count;
}

void debug_proximetre_send_frame(void) {
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

void test_proximetre_module(void) {
    // Test simple pour le proximètre
    proximetre_run_balayage();
    debug_proximetre_send_frame();
}

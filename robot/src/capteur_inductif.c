/**
 * @file capteur_inductif.c
 * @brief Fichier du module capteur_inductif.
 */

#include "capteur_inductif.h"

#include <stdio.h>

#include "LPC17xx.h"

#include "adc.h"
#include "decode_enveloppe.h"
#include "robot_state.h"
#include "timers.h"
#include "uart.h"

/* ==========================================================================
 * CONFIGURATION DE LA MÉTROLOGIE (TIMER 2)
 * ========================================================================== */
#define PCLK_FREQ_HZ         25000000UL  // Fréquence PCLK estimée à 25 MHz
#define TIMER2_TICK_US       10          // Résolution du timer désirée en microsecondes (10 us = 0.01 ms)
#define TIMER2_PRESCALER     ((PCLK_FREQ_HZ / (1000000UL / TIMER2_TICK_US)) - 1)

#define CAPTEUR_IND_ADC_CH_AV  1u
#define CAPTEUR_IND_ADC_CH_AR  2u
#define CAPTEUR_IND_ADC_CH_HOR 3u

/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */
static uint8_t ind_hw_mode = 0b111; // Mode matériel actuel (par défaut 111)
static uint8_t ind_wire_pending = 0;
static uint8_t ind_wire_mode = 0;

// Variables pour la mesure d'enveloppe
static uint32_t last_rise_time = 0;           // Temps du dernier front montant
static uint32_t last_fall_time = 0;           // Temps du dernier front descendant
static uint16_t last_rest_duration_us = 500;  // Durée du repos bas (cycle précédent)
static uint16_t current_period_us = 0;

// FIFO d'événements pour l'enveloppe
#define ENVELOPE_FIFO_SIZE 32
typedef struct {
    uint16_t period_us;
    uint16_t rest_us;
} EnvelopeEvent_t;

static volatile EnvelopeEvent_t env_fifo[ENVELOPE_FIFO_SIZE];
static volatile uint8_t fifo_head = 0;
static volatile uint8_t fifo_tail = 0;

volatile uint32_t volatile_adc_sum1 = 0;
volatile uint32_t volatile_adc_sum2 = 0;
volatile uint32_t volatile_adc_sum3 = 0;
volatile uint16_t volatile_adc_sample_count = 0;

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void init_switches(void);
static void init_adc(void);
static void init_clock(void);
static void init_enveloppe(void);
static void update_switch_mode_from_gpio(void);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

static void init_switches(void) {
    // Configurer P0.0, P0.1 et P0.6 en GPIO (PINSEL0 bits 0:1, 2:3, 12:13)
    LPC_PINCON->PINSEL0 &= ~((3u << 0) | (3u << 2) | (3u << 12));
    LPC_PINCON->PINMODE0 &= ~((3u << 0) | (3u << 2) | (3u << 12)); // Pull-up par défaut
    
    // Direction entrée
    LPC_GPIO0->FIODIR &= ~(PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);

    // Interruption sur front montant et descendant (EINT3 partagée sur tout le port 0)
    LPC_GPIOINT->IO0IntEnR |= (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);
    LPC_GPIOINT->IO0IntEnF |= (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);
}

static void init_adc(void) {
    adc_pin_config(CAPTEUR_IND_ADC_CH_AV);
    adc_pin_config(CAPTEUR_IND_ADC_CH_AR);
    adc_pin_config(CAPTEUR_IND_ADC_CH_HOR);

    // Enable ADC IRQ in NVIC
    NVIC_EnableIRQ(ADC_IRQn);
}

static void init_clock(void) {
    // Configurer P0.27 (clock) en entrée GPIO.
    LPC_PINCON->PINSEL1 &= ~(3u << 22);
    LPC_GPIO0->FIODIR &= ~CAPTEUR_IND_SW_CLOCK;

    // Interruption sur front descendant
    LPC_GPIOINT->IO0IntEnF |= CAPTEUR_IND_SW_CLOCK;
}

static void init_enveloppe(void) {
    // Configurer P0.28 (enveloppe) en entrée GPIO.
    LPC_PINCON->PINSEL1 &= ~(3u << 24);
    LPC_GPIO0->FIODIR &= ~CAPTEUR_IND_SW_ENVELOP;

    // Interruptions sur P0.28 : front montant et descendant.
    LPC_GPIOINT->IO0IntEnR |= CAPTEUR_IND_SW_ENVELOP;
    LPC_GPIOINT->IO0IntEnF |= CAPTEUR_IND_SW_ENVELOP;

    // Configurer le Timer 2 pour mesurer la période (via l'abstraction timers.h)
    timer2_init_free_running(TIMER2_PRESCALER);
}

void init_capteur_inductif(void) {
    init_adc();
    init_clock();
    init_enveloppe();
    init_switches();

    ind_hw_mode =  (LPC_GPIO0->FIOPIN & PIN_IND_SW3) ? (1 << 2) : 0;
    ind_hw_mode |= (LPC_GPIO0->FIOPIN & PIN_IND_SW2) ? (1 << 1) : 0;
    ind_hw_mode |= (LPC_GPIO0->FIOPIN & PIN_IND_SW1) ? 1 : 0;

    // Activer l'interruption partagée EINT3 au niveau NVIC.
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void update_switch_mode_from_gpio(void) {
    uint8_t sw1 = (LPC_GPIO0->FIOPIN & PIN_IND_SW1) ? 1 : 0;
    uint8_t sw2 = (LPC_GPIO0->FIOPIN & PIN_IND_SW2) ? 1 : 0;
    uint8_t sw3 = (LPC_GPIO0->FIOPIN & PIN_IND_SW3) ? 1 : 0;

    ind_hw_mode = (sw3 << 2) | (sw2 << 1) | sw1;
}

void capteur_inductif_interrupt_routine(void) {
    // Gestion des switchs
    if (LPC_GPIOINT->IO0IntStatR & (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3) ||
        LPC_GPIOINT->IO0IntStatF & (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3)) {
        update_switch_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);
    }

    if (LPC_GPIOINT->IO0IntStatF & CAPTEUR_IND_SW_CLOCK) {
        // On lance la séquence ADC UNIQUEMENT quand l'enveloppe est à l'état haut
        if (LPC_GPIO0->FIOPIN & CAPTEUR_IND_SW_ENVELOP) {
            adc_start_inductif_sequence();
        }
        LPC_GPIOINT->IO0IntClr = CAPTEUR_IND_SW_CLOCK; // Acquitter
    }

    // P0.28 (enveloppe) : front montant (début de la salve).
    if (LPC_GPIOINT->IO0IntStatR & CAPTEUR_IND_SW_ENVELOP) {
        uint32_t current_time = timer2_get_tc();

        if (last_fall_time > 0) {
            uint32_t rest_ticks = (current_time >= last_fall_time) ?
                (current_time - last_fall_time) :
                (0xFFFFFFFF - last_fall_time) + current_time;
            last_rest_duration_us = (uint16_t)(rest_ticks * (uint32_t)TIMER2_TICK_US);
        }

        last_rise_time = current_time;

        // Note : On ne remet plus les sommes ADC à zéro ici.
        // Elles s'accumulent sur toute la période de 20ms pour être moyennées dans le main.

        LPC_GPIOINT->IO0IntClr = CAPTEUR_IND_SW_ENVELOP; // Acquitter
    }

    // P0.28 (enveloppe) : front descendant (fin de la salve).
    if (LPC_GPIOINT->IO0IntStatF & CAPTEUR_IND_SW_ENVELOP) {
        uint32_t current_time = timer2_get_tc();

        uint32_t period_ticks = (current_time >= last_rise_time) ?
            (current_time - last_rise_time) :
            (0xFFFFFFFF - last_rise_time) + current_time;

        if (period_ticks > 0) {
            uint32_t period_us = period_ticks * (uint32_t)TIMER2_TICK_US;
            current_period_us = (uint16_t)period_us;
            
            // Pousser dans la FIFO (Uniquement les informations temporelles)
            uint8_t next_head = (fifo_head + 1) % ENVELOPE_FIFO_SIZE;
            if (next_head != fifo_tail) {
                env_fifo[fifo_head].period_us = (uint16_t)period_us;
                env_fifo[fifo_head].rest_us = last_rest_duration_us;
                fifo_head = next_head;
            }
        }

        last_fall_time = current_time;

        LPC_GPIOINT->IO0IntClr = CAPTEUR_IND_SW_ENVELOP; // Acquitter
    }
}

void capteur_inductif_receive_wire_command(uint8_t wire_code) {
    if (wire_code == 0b110 || wire_code == 0b111) {
        ind_wire_mode = wire_code & 0x1u;
        ind_wire_pending = 1;
    }
}

uint16_t get_envelope_period_us(void) {
    if (SIMULATE_SENSOR_VALUES) {
        return 2500; // 2.5 ms
    } else {
        return current_period_us;
    }
}

void capteur_inductif_update(void) {
    EnvelopeEvent_t ev;
    
    // 1. Traitement des événements d'enveloppe (Dépilement de la FIFO)
    while (fifo_tail != fifo_head) {
        NVIC_DisableIRQ(EINT3_IRQn);
        ev = env_fifo[fifo_tail];
        fifo_tail = (fifo_tail + 1) % ENVELOPE_FIFO_SIZE;
        NVIC_EnableIRQ(EINT3_IRQn);

        // Transmission au décodeur d'enveloppe
        decode_enveloppe_commande(ev.period_us, ev.rest_us);
    }

    // 2. Traitement global des ADC pour l'estimation de distance (Indépendant de la trame)
    // On copie et on reset les sommes accumulées pendant les 20 dernières millisecondes
    NVIC_DisableIRQ(ADC_IRQn);
    uint32_t sum1 = volatile_adc_sum1;
    uint32_t sum2 = volatile_adc_sum2;
    uint32_t sum3 = volatile_adc_sum3;
    uint16_t count = volatile_adc_sample_count;
    
    volatile_adc_sum1 = 0;
    volatile_adc_sum2 = 0;
    volatile_adc_sum3 = 0;
    volatile_adc_sample_count = 0;
    NVIC_EnableIRQ(ADC_IRQn);

    // Calcul de la moyenne hors interruption
    if (count > 0) {
        uint16_t avg1 = (uint16_t)(sum1 / count);
        uint16_t avg2 = (uint16_t)(sum2 / count);
        uint16_t avg3 = (uint16_t)(sum3 / count);
        set_capteur_averages(avg1, avg2, avg3);
    }
}

void debug_inductif_send_frame(void) {
    char buffer[128];
    uint16_t avg_av = 0, avg_ar = 0, avg_hor = 0;
    int32_t dist_av = 0, dist_ar = 0, dist_mil = 0, angle = 0;
    uint16_t period_us = get_envelope_period_us();

    if (ind_hw_mode == 0b111) {
        if (!ind_wire_pending) return;
        ind_wire_pending = 0;
    }

    switch (ind_hw_mode) {
        case 0b000:
            sprintf(buffer, "I %ld\r\n", (long)period_us);
            break;
        case 0b001:
            get_inductif_values(&dist_av, &dist_ar, &dist_mil, &angle);
            sprintf(buffer, "a %ld°\r\n", (long)angle);
            break;
        case 0b010:
            get_inductif_values(&dist_av, &dist_ar, &dist_mil, &angle);
            sprintf(buffer, "X%ldmm\r\n", (long)dist_ar);
            break;
        case 0b011:
            get_inductif_values(&dist_av, &dist_ar, &dist_mil, &angle);
            sprintf(buffer, "x%ldmm\r\n", (long)dist_av);
            break;
        case 0b100:
            get_capteur_averages(&avg_av, &avg_ar, &avg_hor);
            sprintf(buffer, "C 0x%03lX\r\n", (unsigned long)avg_hor);
            break;
        case 0b101:
            get_capteur_averages(&avg_av, &avg_ar, &avg_hor);
            sprintf(buffer, "B 0x%03lX\r\n", (unsigned long)avg_ar);
            break;
        case 0b110:
            get_capteur_averages(&avg_av, &avg_ar, &avg_hor);
            sprintf(buffer, "A 0x%03lX\r\n", (unsigned long)avg_av);
            break;
        case 0b111:
            sprintf(buffer, "W %d\r\n", ind_wire_mode);
            break;
        default:
            buffer[0] = '\0';
            break;
    }

    if (buffer[0] != '\0') {
        uart0_send_string(buffer);
    }
}



void test_capteur_inductif(void) {
    // Test simple pour vérifier le module (à décommenter dans le main)
    debug_inductif_send_frame();
}

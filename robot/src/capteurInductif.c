#include "LPC17xx.h"
#include "capteurInductif.h"
#include "robotState.h"
#include "uart.h"
#include "decode_enveloppe.h"
#include <stdio.h>

// --- Configuration de la métrologie (Timer 2) ---
#define PCLK_FREQ_HZ         25000000UL  // Fréquence PCLK estimée à 25 MHz
#define TIMER2_TICK_US       10          // Résolution du timer désirée en microsecondes (10 us = 0.01 ms)
#define TIMER2_PRESCALER     ((PCLK_FREQ_HZ / (1000000UL / TIMER2_TICK_US)) - 1)
// ------------------------------------------------

#define PIN_IND_SW1 (1 << 13) // P0.13
#define PIN_IND_SW2 (1 << 14) // P0.14
#define PIN_IND_SW3 (1 << 15) // P0.15

static uint8_t ind_hw_mode = 0b111; // Mode matériel actuel (par défaut 111)
static uint8_t ind_wire_pending = 0;
static uint8_t ind_wire_mode = 0;

// Variables pour l'accumulation
static uint32_t sum_sin1 = 0;
static uint32_t sum_sin2 = 0;
static uint32_t sum_sin3 = 0;
static uint16_t sample_count = 0;

// Variables pour la mesure d'enveloppe
static uint32_t last_rise_time = 0;      // Temps du dernier front montant
static uint32_t last_fall_time = 0;      // Temps du dernier front descendant
static uint16_t last_rest_duration_us = 500; // Durée du repos bas (cycle précédent)
static uint16_t current_period_us = 0;
static uint16_t current_r_us = 0;

static uint16_t adc_read(uint8_t channel);

static void init_switches(void)
{
    LPC_PINCON->PINSEL0 &= ~((3 << 26) | (3 << 28) | (3 << 30));
    LPC_GPIO0->FIODIR &= ~(PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);

    LPC_GPIOINT->IO0IntEnR |= (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);
    LPC_GPIOINT->IO0IntEnF |= (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);
}

static void init_adc(void)
{
    // 1. Allumer le module ADC
    LPC_SC->PCONP |= (1 << 12);

    // 2. Configurer les broches P0.24 (AD0.1), P0.25 (AD0.2), P0.26 (AD0.3)
    // PINSEL1 bits [17:16], [19:18], [21:20] valent 01
    LPC_PINCON->PINSEL1 &= ~((3 << 16) | (3 << 18) | (3 << 20));
    LPC_PINCON->PINSEL1 |=  ((1 << 16) | (1 << 18) | (1 << 20));

    // Désactiver les résistances de pull-up/pull-down (PINMODE1 = 10)
    LPC_PINCON->PINMODE1 &= ~((3 << 16) | (3 << 18) | (3 << 20));
    LPC_PINCON->PINMODE1 |=  ((2 << 16) | (2 << 18) | (2 << 20));

    // Configuration de l'ADC: Actif (PDN=1), CLKDIV=1 (Dépend de la clock système)
    LPC_ADC->ADCR = (1 << 21) | (1 << 8);
}

static void init_clock(void)
{
    // Configurer P0.27 (Clock) en GPIO entrée
    LPC_PINCON->PINSEL1 &= ~(3 << 22);
    LPC_GPIO0->FIODIR &= ~(1 << 27);

    // Interruptions: P0.27 sur front descendant
    LPC_GPIOINT->IO0IntEnF |= (1 << 27); // Descendant pour la clock de salve
}

static void init_enveloppe(void)
{
    // Configurer P0.28 (Enveloppe) en GPIO entrée
    LPC_PINCON->PINSEL1 &= ~(3 << 24);
    LPC_GPIO0->FIODIR &= ~(1 << 28);

    // Interruptions: P0.28 sur front montant et descendant
    LPC_GPIOINT->IO0IntEnR |= (1 << 28); // Montant pour le début de l'enveloppe
    LPC_GPIOINT->IO0IntEnF |= (1 << 28); // Descendant pour la fin de l'enveloppe

    // Configurer le Timer 2 pour mesurer la période avec une résolution ajustée
    LPC_SC->PCONP |= (1 << 22);      // Allumer Timer 2
    LPC_TIM2->TCR = 2;               // Reset du timer

    // Configuration modulable via les defines :
    // Avec un PCLK à 25MHz et une résolution désirée de 10us, le prescaler calcule automatiquement (250-1).
    // Avec un tick de 10us, un timer 32-bits met ~11.9 heures avant de reboucler (overflow).
    LPC_TIM2->PR = TIMER2_PRESCALER;

    LPC_TIM2->TCR = 1;               // Lancer le timer
}

void init_capteur_inductif(void)
{
    init_adc();
    init_clock();
    init_enveloppe();
    init_switches();

    ind_hw_mode =  LPC_GPIO0->FIOPIN & PIN_IND_SW3 << 2;
    ind_hw_mode |= LPC_GPIO0->FIOPIN & PIN_IND_SW2 << 1;
    ind_hw_mode |= LPC_GPIO0->FIOPIN & PIN_IND_SW1;

    // Activer l'interruption partagée EINT3 au niveau NVIC
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void update_switch_mode_from_gpio(void)
{
    uint8_t sw1 = LPC_GPIO0->FIOPIN & PIN_IND_SW1;
    uint8_t sw2 = LPC_GPIO0->FIOPIN & PIN_IND_SW2;
    uint8_t sw3 = LPC_GPIO0->FIOPIN & PIN_IND_SW3;

    ind_hw_mode = (sw3 << 2) | (sw2 << 1) | sw1;
}

void capteurInductif_interrupt_routine(void)
{
    if (LPC_GPIOINT->IO0IntStatR & (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3) ||
        LPC_GPIOINT->IO0IntStatF & (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3))
    {
        update_switch_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_IND_SW1 | PIN_IND_SW2 | PIN_IND_SW3);
    }

    if (LPC_GPIOINT->IO0IntStatF & (1 << 27))
    {
        // On récupère une valeur sur chaque entrée analogique
        sum_sin1 += adc_read(1);
        sum_sin2 += adc_read(2);
        sum_sin3 += adc_read(3);
        sample_count++;

        LPC_GPIOINT->IO0IntClr = (1 << 27); // Acquitter
    }

    // P0.28 (Enveloppe) : Front Montant (Début de la salve/pulse)
    if (LPC_GPIOINT->IO0IntStatR & (1 << 28))
    {
        uint32_t current_time = LPC_TIM2->TC;

        // Mesurer la durée bas du cycle précédent (si first_fall_time existe)
        if (last_fall_time > 0) {
            uint32_t rest_ticks = (current_time >= last_fall_time) ?
                (current_time - last_fall_time) :
                (0xFFFFFFFF - last_fall_time) + current_time;
            last_rest_duration_us = (uint16_t)(rest_ticks * (uint32_t)TIMER2_TICK_US);
        }

        last_rise_time = current_time;

        // Remise à zéro des accumulateurs pour cette nouvelle salve
        sum_sin1 = 0;
        sum_sin2 = 0;
        sum_sin3 = 0;
        sample_count = 0;

        LPC_GPIOINT->IO0IntClr = (1 << 28); // Acquitter Front Montant
    }

    // P0.28 (Enveloppe) : Front Descendant (Fin de la salve/pulse)
    if (LPC_GPIOINT->IO0IntStatF & (1 << 28))
    {
        uint32_t current_time = LPC_TIM2->TC;

        // Calculer la durée (période en ticks de 10us) passée à l'état haut
        uint32_t period_ticks = (current_time >= last_rise_time) ?
            (current_time - last_rise_time) :
            (0xFFFFFFFF - last_rise_time) + current_time;

        if (period_ticks > 0) {
            uint32_t period_us = period_ticks * (uint32_t)TIMER2_TICK_US;
            current_period_us = (uint16_t)period_us;

            // Appel du décodeur d'enveloppe avec période haute et durée bas (du cycle précédent)
            decode_enveloppe_commande(current_period_us, last_rest_duration_us);
        }

        last_fall_time = current_time;

        // C'est également la fin de la salve, on fige les moyennes glissantes calculées
        if (sample_count > 0)
        {
            uint16_t avg_av = (uint16_t)(sum_sin1 / sample_count);
            uint16_t avg_ar = (uint16_t)(sum_sin2 / sample_count);
            uint16_t avg_hor = (uint16_t)(sum_sin3 / sample_count);

            // Mettre à jour l'état global via robotState (accesseurs centralisés)
            set_capteur_averages(avg_av, avg_ar, avg_hor);
        }

        LPC_GPIOINT->IO0IntClr = (1 << 28); // Acquitter Front Descendant
    }
}

void capteurInductif_receive_wire_command(uint8_t wire_code)
{
    if (wire_code == 0b110 || wire_code == 0b111) {
        ind_wire_mode = wire_code & 0x1u;
        ind_wire_pending = 1;
    }
}

// Fonction utilitaire pour lire un canal ADC manuellement
// On utilise une méthode bloquante pour lire rapidement les 3 signaux l'un après l'autre
static uint16_t adc_read(uint8_t channel)
{
    // Nettoyer le canal puis sélectionner le nouveau
    LPC_ADC->ADCR &= ~(0xFF);
    LPC_ADC->ADCR |= (1 << channel);

    // Démarrer la conversion (Start = 001)
    LPC_ADC->ADCR |= (1 << 24);

    // Attendre que la conversion soit terminée (Flag DONE = bit 31)
    while (!(LPC_ADC->ADGDR & (1UL << 31)));

    // Récupérer la valeur 12 bits (bits 15:4)
    uint16_t val = (LPC_ADC->ADGDR >> 4) & 0xFFF;

    // Stopper l'ADC
    LPC_ADC->ADCR &= ~(7 << 24);

    return val;
}

/* Les accesseurs sont fournis par `robotState` :
   - `get_capteur_averages()` et `get_inductif_values()` sont définis dans robotState.c
   Le module capteurInductif publie uniquement `set_capteur_averages()` via
   l'appel dans l'interruption lorsque de nouvelles moyennes sont disponibles.
*/

uint16_t get_envelope_period_us(void)
{
#if SIMULATE_SENSOR_VALUES
    return 2500; // 2.5 ms -> 2500 us
#else
    return current_period_us;
#endif
}

void debug_inductif_send_frame(void)
{
    char buffer[128];
    uint16_t avg_av = 0;
    uint16_t avg_ar = 0;
    uint16_t avg_hor = 0;
    int32_t dist_av = 0;
    int32_t dist_ar = 0;
    int32_t dist_mil = 0;
    int32_t angle = 0;
    uint16_t period_us = get_envelope_period_us();

    // Mode 0b111: wire command mode - only respond if command pending
    if (ind_hw_mode == 0b111)
    {
        if (!ind_wire_pending) {
            return;
        }
        ind_wire_pending = 0;  // Clear pending flag after handling
    }

    switch (ind_hw_mode)
    {
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
            // Wire command mode response
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

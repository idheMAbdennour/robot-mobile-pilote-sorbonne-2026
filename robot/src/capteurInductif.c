#include "LPC17xx.h"
#include "capteurInductif.h"

// --- Configuration de la métrologie (Timer 2) ---
#define PCLK_FREQ_HZ         25000000UL  // Fréquence PCLK estimée à 25 MHz
#define TIMER2_TICK_US       10UL        // Résolution du timer désirée en microsecondes (10 us = 0.01 ms)
#define TIMER2_PRESCALER     ((PCLK_FREQ_HZ / (1000000UL / TIMER2_TICK_US)) - 1)
#define TIMER2_TICK_TO_MS    ((float)TIMER2_TICK_US / 1000.0f)
// ------------------------------------------------

// Variables pour l'accumulation
static uint32_t sum_sin1 = 0;
static uint32_t sum_sin2 = 0;
static uint32_t sum_sin3 = 0;
static uint32_t sample_count = 0;

// Variables pour les valeurs moyennes lues
static uint32_t current_avg1 = 0;
static uint32_t current_avg2 = 0;
static uint32_t current_avg3 = 0;

// Variables pour la mesure d'enveloppe
static uint32_t last_capture_time = 0;
static uint32_t envelope_period_ticks = 0;
static float current_period_ms = 0.0f;

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

    // Activer l'interruption partagée EINT3 au niveau NVIC
    NVIC_EnableIRQ(EINT3_IRQn);
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

// Cette fonction est appelée par EINT3_IRQHandler dans interruptions.c
void capteurInductif_interrupt_routine(void)
{
    // P0.27 (Clock) : Front Descendant
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
        // Enregistrer le temps de départ
        last_capture_time = LPC_TIM2->TC;

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
        if (current_time >= last_capture_time) {
            envelope_period_ticks = current_time - last_capture_time;
        } else {
            envelope_period_ticks = (0xFFFFFFFF - last_capture_time) + current_time;
        }

        if (envelope_period_ticks > 0) {
            // Conversion en ms dynamique en fonction du define (ex: 1 tick = 0.01 ms)
            current_period_ms = (float)envelope_period_ticks * TIMER2_TICK_TO_MS;
        }

        // C'est également la fin de la salve, on fige les moyennes glissantes calculées
        if (sample_count > 0)
        {
            current_avg1 = sum_sin1 / sample_count;
            current_avg2 = sum_sin2 / sample_count;
            current_avg3 = sum_sin3 / sample_count;
        }

        LPC_GPIOINT->IO0IntClr = (1 << 28); // Acquitter Front Descendant
    }
}

void get_capteur_averages(uint32_t *avg1, uint32_t *avg2, uint32_t *avg3)
{
    if (avg1) *avg1 = current_avg1;
    if (avg2) *avg2 = current_avg2;
    if (avg3) *avg3 = current_avg3;
}

float get_envelope_period_ms(void)
{
    return current_period_ms;
}

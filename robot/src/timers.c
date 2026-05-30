/**
 * @file timers.c
 * @brief Fichier du module timers.
 */

#include "timers.h"

/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

// --- TIMER 0 : Enveloppe IR ---
void timer0_init(uint16_t delai_us) {
    // 1. Allumer le Timer 0
    LPC_SC->PCONP |= (1 << 1);

    // 2. Horloge pour Timer 0 (CCLK/4 -> 25MHz si CCLK = 100MHz)
    LPC_SC->PCLKSEL0 &= ~(3 << 2);

    // 3. Configuration du Timer
    LPC_TIM0->PR = 25 - 1; // Prescaler pour obtenir 1us à 25MHz
    LPC_TIM0->MR0 = delai_us - 1; // ticks = delai_us

    // 4. Reset & Interrupt sur Match 0
    LPC_TIM0->MCR |= (1 << 0) | (1 << 1);
    LPC_TIM0->TCR &= ~(1 << 2); // Clear stop

    // 5. Activer l'interruption au niveau du NVIC
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void timer0_start(void) {
    LPC_TIM0->TCR = (1 << 1); // Reset
    LPC_TIM0->TCR = (1 << 0); // Start
}

void timer0_stop(void) {
    LPC_TIM0->TCR &= ~(1 << 0); // Stop
}

// --- TIMER 2 : Capteur Inductif ---
void timer2_init_free_running(uint32_t prescaler) {
    // 1. Allumer le Timer 2
    LPC_SC->PCONP |= (1 << 22);

    // 2. Horloge pour Timer 2 (CCLK/4 -> 25MHz)
    LPC_SC->PCLKSEL1 &= ~(3 << 12);

    // 3. Réinitialiser et configurer
    LPC_TIM2->TCR = 2; // Reset
    LPC_TIM2->PR = prescaler; // Prédiviseur
    
    // 4. Démarrer le Timer en mode libre
    LPC_TIM2->TCR = 1; // Start
}

uint32_t timer2_get_tc(void) {
    return LPC_TIM2->TC;
}

// --- TIMER 3 : Proximètre ---
void timer3_init_servo(uint32_t periode_us, uint32_t pulse_us) {
    // 1. Allumer le Timer 3
    LPC_SC->PCONP |= (1 << 23);

    // 2. Horloge pour Timer 3 (CCLK/4 -> 25MHz)
    LPC_SC->PCLKSEL1 &= ~(3 << 14);

    // 3. Configuration de base
    LPC_TIM3->TCR = 0x02; // Reset
    LPC_TIM3->PR  = 25 - 1; // Prescaler pour 1us à 25MHz
    
    // 4. Match registers pour le PWM logiciel
    LPC_TIM3->MCR = (1 << 0) | (1 << 3); // Interrupt sur MR0 et MR1
    LPC_TIM3->MR0 = 1000; // Première interruption (base)
    LPC_TIM3->MR1 = 1000 + pulse_us; // Deuxième interruption (fin de pulse)
    
    // 5. Nettoyer les interruptions existantes et démarrer
    LPC_TIM3->IR  = 0x3F;
    LPC_TIM3->TCR = 0x01; // Start

    // 6. Activer l'interruption
    NVIC_EnableIRQ(TIMER3_IRQn);
}

uint32_t timer3_get_tc(void) {
    return LPC_TIM3->TC;
}

uint32_t timer3_get_match0(void) {
    return LPC_TIM3->MR0;
}

void timer3_set_match0(uint32_t val) {
    LPC_TIM3->MR0 = val;
}

void timer3_set_match1(uint32_t val) {
    LPC_TIM3->MR1 = val;
}

void timer3_clear_interrupt(uint32_t flag) {
    LPC_TIM3->IR = flag;
}

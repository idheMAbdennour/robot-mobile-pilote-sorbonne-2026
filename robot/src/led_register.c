/**
 * @file led_register.c
 * @brief Fichier du module led_register.
 */

#include "led_register.h"

/* ==========================================================================
 * VARIABLES GLOBALES PRIVÉES
 * ========================================================================== */
static uint16_t current_led_state = 0;

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void update_shift_register(void) {
    // Abaisser LATCH (P0.20)
    LPC_GPIO0->FIOCLR = PIN_LED_SHIFT_LATCH;
    
    // Envoyer les 16 bits (MSB en premier ou LSB selon le câblage, typiquement MSB)
    for (int i = 15; i >= 0; i--) {
        // Abaisser CLK (P4.29)
        LPC_GPIO4->FIOCLR = PIN_LED_SHIFT_CLK;
        
        // Mettre la data (P2.9)
        if (current_led_state & (1 << i)) {
            LPC_GPIO2->FIOSET = PIN_LED_SHIFT_DATA;
        } else {
            LPC_GPIO2->FIOCLR = PIN_LED_SHIFT_DATA;
        }
        
        // Monter CLK pour valider le bit
        LPC_GPIO4->FIOSET = PIN_LED_SHIFT_CLK;
    }
    
    // Monter LATCH pour appliquer les sorties
    LPC_GPIO0->FIOSET = PIN_LED_SHIFT_LATCH;
}

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PUBLIQUES
 * ========================================================================== */
void init_led_register(void) {
    // Configurer les broches en sortie
    LPC_GPIO2->FIODIR |= PIN_LED_SHIFT_DATA;
    LPC_GPIO4->FIODIR |= PIN_LED_SHIFT_CLK;
    LPC_GPIO0->FIODIR |= PIN_LED_SHIFT_LATCH;
    
    // Initialiser les niveaux bas
    LPC_GPIO2->FIOCLR = PIN_LED_SHIFT_DATA;
    LPC_GPIO4->FIOCLR = PIN_LED_SHIFT_CLK;
    LPC_GPIO0->FIOCLR = PIN_LED_SHIFT_LATCH;
    
    current_led_state = 0;
    update_shift_register();
}

void led_register_set(uint16_t mask) {
    current_led_state |= mask;
    update_shift_register();
}

void led_register_clr(uint16_t mask) {
    current_led_state &= ~mask;
    update_shift_register();
}

void led_register_write_all(uint16_t state) {
    current_led_state = state;
    update_shift_register();
}

uint16_t led_register_get_state(void) {
    return current_led_state;
}

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
// Petit délai pour la série 4000 (CD4015) qui est très lente (particulièrement en 3.3V)
static void delay_cd4015(void) {
    for (volatile int d = 0; d < 100; d++); // Délai d'environ 1-2 µs
}

static void update_shift_register(void) {
    // Le CD4015 n'a pas de LATCH (les sorties changent en temps réel).
    // P0.20 est connecté au RST et est maintenu à 0 par init_led_register().
    
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
        
        delay_cd4015(); // Setup time : laisser le temps au bit de data de se stabiliser
        
        // Monter CLK pour décaler le bit
        LPC_GPIO4->FIOSET = PIN_LED_SHIFT_CLK;
        
        delay_cd4015(); // Pulse width : maintenir l'horloge assez longtemps à l'état haut
    }
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

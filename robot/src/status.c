/**
 * @file status.c
 * @brief Fichier du module status.
 */

#include "status.h"

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void delay_test(int msec);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

static void delay_test(int msec) {
    volatile uint16_t count = 0;
    for (int i = 0; i < msec * 1000000; i++) {
        count++;
    }
}

void init_status_led(void) {
    // Configurer P0.22, P3.25, P3.26 en GPIO
    LPC_PINCON->PINSEL1 &= ~(3 << 12); // P0.22 -> Rouge
    LPC_PINCON->PINSEL7 &= ~(3 << 18); // P3.25 -> Vert
    LPC_PINCON->PINSEL7 &= ~(3 << 20); // P3.26 -> Bleu

    // GPIOs en sortie
    LPC_GPIO0->FIODIR |= PIN_LED_R;
    LPC_GPIO3->FIODIR |= PIN_LED_G | PIN_LED_B; // G et B sont sur le même port

    // GPIOs à 0 par défaut (LEDs éteintes)
    LPC_GPIO0->FIOCLR = PIN_LED_R;
    LPC_GPIO3->FIOCLR = PIN_LED_G | PIN_LED_B;
}

void set_status_led(robot_status_t status) {
    // Éteindre toutes les LEDs
    LPC_GPIO0->FIOCLR = PIN_LED_R;
    LPC_GPIO3->FIOCLR = PIN_LED_G | PIN_LED_B;

    switch (status) {
        case STATUS_LIBRE:
            LPC_GPIO3->FIOSET = PIN_LED_G; // Vert
            break;
        case STATUS_RDV_EXPEDITION:
            LPC_GPIO0->FIOSET = PIN_LED_R; // Rouge
            break;
        case STATUS_COLISPRIS:
            LPC_GPIO0->FIOSET = PIN_LED_R;
            LPC_GPIO3->FIOSET = PIN_LED_B; // Magenta (Rouge + Bleu)
            break;
        case STATUS_RDV_DEPOSE:
            LPC_GPIO3->FIOSET = PIN_LED_B; // Bleu
            break;
        default:
            // Par défaut éteint
            break;
    }
}

void test_status_module(void) {
    init_status_led();
    
    while(1) {
        set_status_led(STATUS_LIBRE);
        delay_test(5);
        set_status_led(STATUS_RDV_EXPEDITION);
        delay_test(5);
        set_status_led(STATUS_COLISPRIS);
        delay_test(5);
        set_status_led(STATUS_RDV_DEPOSE);
        delay_test(5);
    }
}

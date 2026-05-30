/**
 * @file buttons.c
 * @brief Fichier du module buttons.
 */

#include "buttons.h"

#include "robot_state.h"

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

void init_buttons(void) {
    // Configuration de P2.6 et P2.7 en tant que GPIO
    LPC_PINCON->PINSEL4 &= ~(3 << 12); // P2.6
    LPC_PINCON->PINSEL4 &= ~(3 << 14); // P2.7
    
    // Configuration en entrée
    LPC_GPIO2->FIODIR &= ~PIN_BTN_LOAD;
    LPC_GPIO2->FIODIR &= ~PIN_BTN_UNLOAD;

    // Configurer les interruptions sur front descendant (appui) pour P2.4 et P2.5
    LPC_GPIOINT->IO2IntEnF |= (PIN_BTN_LOAD | PIN_BTN_UNLOAD);

    // Activer l'interruption EINT3 au niveau NVIC (déjà fait par d'autres modules, mais par sécurité)
    NVIC_EnableIRQ(EINT3_IRQn);
}

void buttons_interrupt_routine(void) {
    if (LPC_GPIOINT->IO2IntStatF & PIN_BTN_LOAD) {
        set_robot_status(STATUS_COLISPRIS);
        LPC_GPIOINT->IO2IntClr = PIN_BTN_LOAD;
    }
    
    if (LPC_GPIOINT->IO2IntStatF & PIN_BTN_UNLOAD) {
        set_robot_status(STATUS_LIBRE);
        LPC_GPIOINT->IO2IntClr = PIN_BTN_UNLOAD;
    }
}

void test_buttons_module(void) {
    // Les boutons sont maintenant gérés par interruption matérielle.
}

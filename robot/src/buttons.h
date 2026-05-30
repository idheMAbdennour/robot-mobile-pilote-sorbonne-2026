/**
 * @file buttons.h
 * @brief Fichier du module buttons.
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "LPC17xx.h"

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_BTN_LOAD   (1 << 6) // P2.6 - Bouton de charge
#define PIN_BTN_UNLOAD (1 << 7) // P2.7 - Bouton de décharge


/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise les broches GPIO pour les boutons.
 */
void init_buttons(void);

/**
 * @brief Gère l'interruption matérielle déclenchée par les boutons.
 */
void buttons_interrupt_routine(void);

/**
 * @brief Fonction de test du module (vide si 100% interruptions)
 */
void test_buttons_module(void);

#endif // BUTTONS_H

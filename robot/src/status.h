/**
 * @file status.h
 * @brief Fichier du module status.
 */

#ifndef STATUS_H
#define STATUS_H

#include "LPC17xx.h"

#include "robot_state.h"

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_LED_R (1 << 22) // P0.22 - LED Rouge
#define PIN_LED_G (1 << 25) // P3.25 - LED Verte
#define PIN_LED_B (1 << 26) // P3.26 - LED Bleue

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise les GPIOs pour la LED RVB d'affichage du statut.
 */
void init_status_led(void);

/**
 * @brief Change la couleur de la LED en fonction du statut du robot.
 * @param status Le nouveau statut (robot_status_t).
 */
void set_status_led(robot_status_t status);

/**
 * @brief Fonction de test du module statut, décommentable dans le main.
 */
void test_status_module(void);

#endif // STATUS_H
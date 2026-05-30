/**
 * @file dtmf.h
 * @brief Fichier du module dtmf.
 */

#ifndef DTMF_H
#define DTMF_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_DTMF_D0         (1 << 16) // P0.16 - Donnée 0 DTMF
#define PIN_DTMF_D1         (1 << 17) // P0.17 - Donnée 1 DTMF
#define PIN_DTMF_D2         (1 << 18) // P0.18 - Donnée 2 DTMF
#define PIN_DTMF_D3         (1 << 19) // P0.19 - Donnée 3 DTMF
#define PIN_DTMF_VALID      (1 << 12) // P2.12 - Signal de validation (Interruption)

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le module DTMF (GPIOs, interruptions).
 */
void init_dtmf(void);

/**
 * @brief Routine d'interruption pour le décodeur DTMF (sur P0.20).
 */
void dtmf_interrupt_routine(void);

/**
 * @brief Traite les commandes DTMF reçues et met à jour l'état (appelé périodiquement).
 */
void dtmf_service(void);

/**
 * @brief Fonction de test du module DTMF, décommentable dans le main.
 */
void test_dtmf_module(void);

#endif // DTMF_H

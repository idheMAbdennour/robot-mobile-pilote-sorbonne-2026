/**
 * @file interruptions.h
 * @brief Fichier du module interruptions.
 */

#ifndef INTERRUPTIONS_H
#define INTERRUPTIONS_H

#include "LPC17xx.h"

/* ==========================================================================
 * PROTOTYPES DES GESTIONNAIRES D'INTERRUPTIONS MATÉRIELLES (CMSIS)
 * ========================================================================== */

/**
 * @brief Gestionnaire d'interruption pour le SysTick.
 */
void SysTick_Handler(void);

/**
 * @brief Gestionnaire d'interruption pour EINT3 (Port 0 et 2 partagés).
 */
void EINT3_IRQHandler(void);

/**
 * @brief Gestionnaire d'interruption pour EINT1 (Dédié).
 */
void EINT1_IRQHandler(void);

/**
 * @brief Gestionnaire d'interruption pour le Timer 0.
 */
void TIMER0_IRQHandler(void);

/**
 * @brief Gestionnaire d'interruption pour le Timer 3.
 */
void TIMER3_IRQHandler(void);

#endif /* INTERRUPTIONS_H */
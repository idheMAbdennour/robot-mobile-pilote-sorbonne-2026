/**
 * @file timers.h
 * @brief Fichier du module timers.
 */

#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>

#include "LPC17xx.h"

/* ==========================================================================
 * DÉFINITIONS DES TIMERS
 * ========================================================================== */
// Les timers du LPC1768 sont utilisés pour les fonctionnalités suivantes :
// - TIMER 0 : Enveloppe IR (Interruptions toutes les 250us)
// - TIMER 2 : Capteur Inductif (Compteur libre pour mesure de temps)
// - TIMER 3 : Proximètre (Génération de signal PWM logiciel pour le servo et base de temps)

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le Timer 0 pour l'enveloppe IR.
 * @param delai_us Le délai en microsecondes (typiquement 250us).
 */
void timer0_init(uint16_t delai_us);

/**
 * @brief Démarre le Timer 0.
 */
void timer0_start(void);

/**
 * @brief Arrête le Timer 0.
 */
void timer0_stop(void);

/**
 * @brief Réinitialise et démarre le Timer 2 (utilisé par le capteur inductif).
 * @param prescaler Le prédiviseur à utiliser.
 */
void timer2_init_free_running(uint32_t prescaler);

/**
 * @brief Récupère la valeur courante du Timer 2.
 * @return La valeur du compteur (TC).
 */
uint32_t timer2_get_tc(void);

/**
 * @brief Initialise le Timer 3 pour la génération du signal du proximètre.
 * @param periode_us La période en microsecondes.
 * @param pulse_us La durée de l'impulsion en microsecondes.
 */
void timer3_init_servo(uint32_t periode_us, uint32_t pulse_us);

/**
 * @brief Récupère la valeur courante du Timer 3.
 * @return La valeur du compteur (TC).
 */
uint32_t timer3_get_tc(void);

/**
 * @brief Récupère la valeur de Match 0 du Timer 3.
 * @return La valeur de MR0.
 */
uint32_t timer3_get_match0(void);

/**
 * @brief Définit la valeur de Match 0 du Timer 3.
 * @param val La valeur pour MR0.
 */
void timer3_set_match0(uint32_t val);

/**
 * @brief Définit la valeur de Match 1 du Timer 3.
 * @param val La valeur pour MR1.
 */
void timer3_set_match1(uint32_t val);

/**
 * @brief Nettoie le drapeau d'interruption du Timer 3.
 * @param flag Le bit du drapeau à effacer.
 */
void timer3_clear_interrupt(uint32_t flag);

#endif // TIMERS_H

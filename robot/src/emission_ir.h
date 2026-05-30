/**
 * @file emission_ir.h
 * @brief Fichier du module emission_ir.
 */

#ifndef EMISSION_IR_H
#define EMISSION_IR_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_SYNC_IR (1 << 29) // P1.29 - Pin de synchro oscilloscope pour l'entête IR

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le PWM pour l'émission IR (via pwm.h).
 */
void init_pwm_ir(void);

/**
 * @brief Initialise le Timer pour l'enveloppe de modulation IR (via timers.h).
 * @param delai_us Délai en microsecondes (ex: 250us).
 */
void init_timer_enveloppe(uint16_t delai_us);

/**
 * @brief Prépare la séquence d'enveloppe IR à partir des données.
 * @param id Numéro d'identifiant du robot.
 * @param vitesse Vitesse du robot.
 * @param status Statut du robot.
 */
void preparer_trame(uint8_t id, uint8_t vitesse, uint8_t status);

/**
 * @brief Routine d'interruption appelée par le Timer d'enveloppe IR.
 */
void emission_ir_interrupt_routine(void);

/**
 * @brief Fonction de test pour l'émission IR.
 */
void test_emission_ir(void);

#endif // EMISSION_IR_H

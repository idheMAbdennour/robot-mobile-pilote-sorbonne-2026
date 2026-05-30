/**
 * @file pwm.h
 * @brief Fichier du module pwm.
 */

#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#include "LPC17xx.h"

/* ==========================================================================
 * DÉFINITIONS DES PWM
 * ========================================================================== */
// Le module PWM1 est partagé entre :
// - PWM 1.1 : Moteur Gauche (P2.0)
// - PWM 1.2 : Moteur Droit (P2.1)
// - PWM 1.3 : Émission IR 38kHz (P2.2)
//
// Afin d'éviter les conflits, une fréquence de base commune est définie.
// L'IR nécessite obligatoirement 38 kHz. Les moteurs fonctionnent très bien 
// à cette fréquence (inaudible et efficace).
#define PWM_BASE_FREQ_HZ 38000

// Horloge du périphérique PWM (CCLK / 4 avec CCLK = 100MHz)
#define PCLK_PWM 25000000

// Période commune calculée automatiquement pour le registre MR0
// (avec un prescaler PR = 0)
#define PWM_MR0_PERIOD (PCLK_PWM / PWM_BASE_FREQ_HZ)

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le PWM1 pour les moteurs (PWM1.1 et PWM1.2).
 */
void pwm_init_moteurs(void);

/**
 * @brief Met à jour le rapport cyclique du moteur gauche (PWM1.1).
 * @param pourcent Rapport cyclique (0 à 100).
 */
void pwm_set_moteur_gauche(uint8_t pourcent);

/**
 * @brief Met à jour le rapport cyclique du moteur droit (PWM1.2).
 * @param pourcent Rapport cyclique (0 à 100).
 */
void pwm_set_moteur_droit(uint8_t pourcent);

/**
 * @brief Met à jour le rapport cyclique des deux moteurs simultanément.
 * @param pourcent_gauche Rapport cyclique gauche (0 à 100).
 * @param pourcent_droite Rapport cyclique droit (0 à 100).
 */
void pwm_set_moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite);

/**
 * @brief Initialise le PWM1 pour l'émission IR (PWM1.3) à 38kHz.
 */
void pwm_init_ir(void);

/**
 * @brief Active la sortie PWM1.3 pour l'émission IR.
 */
void pwm_enable_ir_output(void);

/**
 * @brief Désactive la sortie PWM1.3 pour l'émission IR.
 */
void pwm_disable_ir_output(void);

/**
 * @brief Réinitialise le compteur du PWM1 (utilisé pour la synchronisation IR).
 */
void pwm_reset_counter_ir(void);

#endif // PWM_H

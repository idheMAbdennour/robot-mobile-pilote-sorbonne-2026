#ifndef MOTEUR_H
#define MOTEUR_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_MOT_SW1 (1 << 4) // P0.4 - Switch moteur 1
#define PIN_MOT_SW2 (1 << 5) // P0.5 - Switch moteur 2


/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le module PWM pour les deux roues du robot (via pwm.c).
 */
void init_moteur_pwm(void);

/**
 * @brief Modifie le rapport cyclique de la roue GAUCHE.
 * @param pourcent Valeur entre 0 (arrêt) et 100 (vitesse max).
 */
void changer_pwm_gauche(uint8_t pourcent);

/**
 * @brief Modifie le rapport cyclique de la roue DROITE.
 * @param pourcent Valeur entre 0 (arrêt) et 100 (vitesse max).
 */
void changer_pwm_droite(uint8_t pourcent);

/**
 * @brief Modifie simultanément la vitesse des deux roues.
 * @param pourcent_gauche Valeur entre 0 et 100.
 * @param pourcent_droite Valeur entre 0 et 100.
 */
void changer_pwm_moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite);

/**
 * @brief Initialise les interruptions et GPIO pour le debug des moteurs.
 */
void init_moteurs_debug(void);

/**
 * @brief Routine d'interruption pour les switchs moteurs.
 */
void moteurs_interrupt_routine(void);
/**
 * @brief Reçoit et traite une commande via le fil (enveloppe).
 * @param wire_code Le code reçu.
 */
void moteurs_receive_wire_command(uint8_t wire_code);
/**
 * @brief Envoie l'état de debug actuel des moteurs via UART.
 */
void debug_moteurs_send_frame(void);

/**
 * @brief Fonction de test du module moteurs, décommentable dans le main.
 */
void test_moteurs_module(void);

#endif // MOTEUR_H

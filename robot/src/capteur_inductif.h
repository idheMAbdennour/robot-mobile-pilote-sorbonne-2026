/**
 * @file capteur_inductif.h
 * @brief Fichier du module capteur_inductif.
 */

#ifndef CAPTEUR_INDUCTIF_H
#define CAPTEUR_INDUCTIF_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_IND_SW1            (1u << 0)  // P0.0 - Switch inductif 1
#define PIN_IND_SW2            (1u << 1)  // P0.1 - Switch inductif 2
#define PIN_IND_SW3            (1u << 6)  // P0.6 - Switch inductif 3
#define CAPTEUR_IND_SW_CLOCK   (1 << 27) // P0.27 - Horloge de salve
#define CAPTEUR_IND_SW_ENVELOP (1 << 28) // P0.28 - Enveloppe


/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le capteur inductif (ADC, GPIO pour l'horloge et l'enveloppe, Timer pour mesure réseau).
 */
void init_capteur_inductif(void);

/**
 * @brief Routine d'interruption à appeler dans EINT3_IRQHandler pour gérer P0.27, P0.28 et les switchs.
 */
void capteur_inductif_interrupt_routine(void);

/**
 * @brief Reçoit un code filaire 1XX et prépare une émission unique si le mode HW l'autorise.
 * @param wire_code Le code filaire reçu.
 */
void capteur_inductif_receive_wire_command(uint8_t wire_code);

/**
 * @brief Traite l'événement d'enveloppe et affiche l'état UART du capteur inductif.
 */
void debug_inductif_send_frame(void);

/**
 * @brief Récupère la dernière période d'enveloppe mesurée.
 * @return Période en microsecondes.
 */
uint16_t get_envelope_period_us(void);

/**
 * @brief Dépile la FIFO des événements d'enveloppe et met à jour le décodeur et l'ADC.
 * Doit être appelée périodiquement (par exemple à 50Hz dans le main).
 */
void capteur_inductif_update(void);

/**
 * @brief Test du module capteur inductif (décommentable dans main).
 */
void test_capteur_inductif(void);

#endif /* CAPTEUR_INDUCTIF_H */

#ifndef CAPTEUR_INDUCTIF_H
#define CAPTEUR_INDUCTIF_H

#include <stdint.h>


#define PIN_IND_SW1 (1 << 13) // P0.13
#define PIN_IND_SW2 (1 << 14) // P0.14
#define PIN_IND_SW3 (1 << 15) // P0.15

/**
 * @brief Initialise le capteur inductif (ADC, GPIO pour l'horloge et l'enveloppe, Timer pour mesure réseau)
 */
void init_capteur_inductif(void);

/**
 * @brief Routine d'interruption à appeler dans EINT3_IRQHandler pour gérer P0.27 et P0.28
 */
void capteurInductif_interrupt_routine(void);

/**
 * @brief Reçoit un code wire 1XX et prépare une émission unique si le mode HW l'autorise.
 */
void capteurInductif_receive_wire_command(uint8_t wire_code);


/**
 * @brief Récupère la période du signal enveloppe (en microsecondes)
 */
uint16_t get_envelope_period_us(void);

// Removed: debug_inductif_send_frame UART helper (unused in trimmed debug)

#endif /* CAPTEUR_INDUCTIF_H */

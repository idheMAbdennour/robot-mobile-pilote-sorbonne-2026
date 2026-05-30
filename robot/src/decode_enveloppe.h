/**
 * @file decode_enveloppe.h
 * @brief Fichier du module decode_enveloppe.
 */

#ifndef DECODE_ENVELOPPE_H
#define DECODE_ENVELOPPE_H

#include <stdint.h>
#include "robot_state.h"

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Décode un nouveau symbole (période) et met à jour les machines d'état Nord et Sud.
 * @param period_us La période mesurée de l'impulsion.
 * @param rest_duration_us La durée de repos l'ayant précédée.
 */
void decode_enveloppe_commande(uint16_t period_us, uint16_t rest_duration_us);

/**
 * @brief Traite une trame complètement reçue et validée selon le cahier des charges.
 * @param trame Pointeur vers la trame à traiter.
 */
void decode_enveloppe_process_command(const wire_trame_t *trame);

/**
 * @brief Affiche l'état brut du buffer de réception courant sur l'UART pour le débogage.
 */
void test_print_buffer(void);

/**
 * @brief Retourne le nombre de bits actuellement collectés (pour le débogage).
 * @return Nombre de bits (0 à 14).
 */
uint8_t decode_enveloppe_get_buffer_index(void);

#endif // DECODE_ENVELOPPE_H

#ifndef DECODE_ENVELOPPE_H
#define DECODE_ENVELOPPE_H

#include <stdint.h>

/**
 * @brief Traduit la période mesurée de l'enveloppe en message ou commande.
 * Accumule les symboles dans des buffers et décode un message complet si possible.
 *
 * @param period_us Période de l'impulsion haute (enveloppe) en microsecondes
 * @param rest_duration_us Durée du repos bas entre deux impulsions en microsecondes (doit être ~500 µs)
 * Modifie potentiellement le robotState et les modes de debugs.
 */
void decode_enveloppe_commande(uint16_t period_us, uint16_t rest_duration_us);

// Debug: affiche l'état brut du buffer de réception courant sur UART.
void decode_enveloppe_debug_print_uart(void);

// Debug: retourne le nombre de symboles accumulés dans le buffer courant.
uint8_t decode_enveloppe_get_buffer_index(void);

#endif // DECODE_ENVELOPPE_H

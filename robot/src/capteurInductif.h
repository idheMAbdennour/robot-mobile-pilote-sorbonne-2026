#ifndef CAPTEUR_INDUCTIF_H
#define CAPTEUR_INDUCTIF_H

#include <stdint.h>

/**
 * @brief Initialise le capteur inductif (ADC, GPIO pour l'horloge et l'enveloppe, Timer pour mesure réseau)
 */
void init_capteur_inductif(void);

/**
 * @brief Routine d'interruption à appeler dans EINT3_IRQHandler pour gérer P0.27 et P0.28
 */
void capteurInductif_interrupt_routine(void);

/**
 * @brief Récupère les dernières valeurs moyennes des capteurs
 */
void get_capteur_averages(uint32_t *avg1, uint32_t *avg2, uint32_t *avg3);

/**
 * @brief Récupère la période du signal enveloppe (en millisecondes)
 */
float get_envelope_period_ms(void);

#endif /* CAPTEUR_INDUCTIF_H */

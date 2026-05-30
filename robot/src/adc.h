/**
 * @file adc.h
 * @brief Fichier du module adc.
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#include "LPC17xx.h"

/* ==========================================================================
 * DÉFINITIONS ET MACROS
 * ========================================================================== */
#define ADC_CLKDIV 4

// Définition des canaux partagés
#define PROXIMETRE_ADC_CH      0u
#define CAPTEUR_IND_ADC_CH_AV  1u
#define CAPTEUR_IND_ADC_CH_AR  2u
#define CAPTEUR_IND_ADC_CH_HOR 3u

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le périphérique ADC.
 *        Met sous tension l'ADC, active son NVIC et configure son diviseur d'horloge.
 * @param clkdiv Diviseur d'horloge.
 */
void adc_init_shared(uint8_t clkdiv);

/**
 * @brief Configure la broche pour un canal donné.
 * @param channel Numéro du canal ADC.
 */
void adc_pin_config(uint8_t channel);

/**
 * @brief Démarre la séquence de conversion asynchrone du capteur inductif.
 * Appelée depuis EINT3_IRQHandler. Coupe la conversion du proximètre si elle était en cours.
 */
void adc_start_inductif_sequence(void);

/**
 * @brief Demande une moyenne de mesures sur le proximètre.
 * @param num_samples Nombre d'échantillons à moyenner.
 */
void adc_request_proximetre_avg(uint16_t num_samples);

/**
 * @brief Vérifie si la mesure demandée pour le proximètre est terminée.
 * @return 1 si prêt, 0 sinon.
 */
int adc_is_proximetre_ready(void);

/**
 * @brief Récupère la valeur moyenne du proximètre et efface le drapeau 'prêt'.
 * @return La valeur moyenne.
 */
uint16_t adc_get_proximetre_value(void);

#endif // ADC_H

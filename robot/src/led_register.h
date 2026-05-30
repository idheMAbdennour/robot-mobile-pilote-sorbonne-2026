/**
 * @file led_register.h
 * @brief Fichier du module led_register.
 */

#ifndef LED_REGISTER_H
#define LED_REGISTER_H

#include <stdint.h>

#include "LPC17xx.h"

/* ==========================================================================
 * DÉFINITIONS DES PINS (74HC595)
 * ========================================================================== */
#define PIN_LED_SHIFT_DATA  (1 << 9)  // P2.9
#define PIN_LED_SHIFT_CLK   (1 << 29) // P4.29
#define PIN_LED_SHIFT_LATCH (1 << 20) // P0.20

/* ==========================================================================
 * MASQUES DES LEDS
 * ========================================================================== */
#define LED_REG_DTMF_CONCERNE    (1 << 0)
#define LED_REG_ETAT_JONCTION    (1 << 1)
#define LED_REG_ID_POSTE_MASK    (0x0F << 2) // Bits 2 à 5
#define LED_REG_ID_POSTE_SHIFT   2
#define LED_REG_CAPA_AVANT       (1 << 6)
#define LED_REG_CAPA_DROITE      (1 << 7)
#define LED_REG_CAPA_ARRIERE     (1 << 8)
#define LED_REG_CAPA_GAUCHE      (1 << 9)
#define LED_REG_CAPA_MASK        (0x0F << 6) // Bits 6 à 9

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise les broches GPIO pour le registre à décalage.
 */
void init_led_register(void);

/**
 * @brief Allume les LEDs spécifiées par le masque.
 * @param mask Masque des LEDs à allumer.
 */
void led_register_set(uint16_t mask);

/**
 * @brief Éteint les LEDs spécifiées par le masque.
 * @param mask Masque des LEDs à éteindre.
 */
void led_register_clr(uint16_t mask);

/**
 * @brief Définit l'état complet du registre (16 bits) et met à jour le driver.
 * @param state Nouvel état complet.
 */
void led_register_write_all(uint16_t state);

/**
 * @brief Récupère l'état courant du registre en mémoire.
 * @return État courant.
 */
uint16_t led_register_get_state(void);

#endif // LED_REGISTER_H

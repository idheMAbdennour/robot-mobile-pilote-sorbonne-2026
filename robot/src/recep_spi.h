/**
 * @file recep_spi.h
 * @brief Fichier du module recep_spi.
 */

#ifndef RECEP_SPI_H
#define RECEP_SPI_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_SPI_CLK  (1 << 7)  // P0.7
#define PIN_SPI_MISO (1 << 8)  // P0.8
#define PIN_SPI_CS1  (1 << 9)  // P0.9
#define PIN_SPI_CS2  (1 << 10) // P0.10
#define PIN_SPI_CS3  (1 << 11) // P0.11
#define PIN_SPI_CS4  (1 << 15) // P0.15
#define PIN_ODO_READY (1 << 11) // P2.11 (EINT1)


/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le module de réception SPI (Horloge, MISO, CS).
 */
void init_recep_spi(void);

/**
 * @brief Demande une valeur pour un Chip Select précis (0 à 3).
 * @param cs_num Numéro du CS (0 = Vg, 1 = Vd, 2 = Pg, 3 = Pd).
 */
void set_spi_cs_val(int cs_num);

/**
 * @brief Routine d'interruption appelée par EINT1_IRQHandler (horloge SPI).
 */
void recep_spi_interrupt_routine(void);

/**
 * @brief Test du module SPI, décommentable dans le main.
 */
void test_recep_spi(void);

#endif // RECEP_SPI_H
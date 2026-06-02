/**
 * @file recep_spi.h
 * @brief Module de gestion de l'odométrie et acquisition SPI (LPC1769)
 */

#ifndef RECEP_SPI_H
#define RECEP_SPI_H

#include <stdint.h>
#include <stdbool.h>

/* ==========================================================================
 * CONFIGURATION DES BROCHES (GPIO PORT 0)
 * ========================================================================== */
#define PIN_SPI_CLK   (1 << 7)   // P0.7
#define PIN_SPI_MISO  (1 << 8)   // P0.8
#define PIN_SPI_CS1   (1 << 9)   // P0.9  (Vg)
#define PIN_SPI_CS2   (1 << 10)  // P0.10 (Vd)
#define PIN_SPI_CS3   (1 << 11)  // P0.11 (Pg)
#define PIN_SPI_CS4   (1 << 15)  // P0.15 (Pd)

/* ==========================================================================
 * STRUCTURE DE DONNÉES UTILISATEUR
 * ========================================================================== */
typedef struct {
    float distance_g;    // Distance parcourue par la roue gauche sur le dernier échantillon (m)
    float distance_d;    // Distance parcourue par la roue droite sur le dernier échantillon (m)
    float vitesse_g;     // Vitesse linéaire roue gauche (m/s)
    float vitesse_d;     // Vitesse linéaire roue droite (m/s)
    float vitesse_robot; // Vitesse moyenne linéaire du robot (m/s)
    bool  data_ready;    // Flag mis à true à chaque rafraîchissement (50Hz)
} odometrie_data_t;

/* ==========================================================================
 * FONCTIONS PUBLIQUES
 * ========================================================================== */

/**
 * @brief Initialise les GPIOs, configure l'interruption EINT1 (P2.11) et le module.
 */
void ODO_Init(void);

/**
 * @brief Récupère une copie sécurisée des dernières données physiques calculées.
 * @param data Pointeur vers la structure qui va recevoir les données.
 */
void ODO_Get_Data(odometrie_data_t *data);

/**
 * @brief Permet de consommer/remettre à zéro manuellement le flag de synchronisation.
 * @return true si une nouvelle mesure est disponible depuis le dernier appel.
 */
bool ODO_Check_New_Data(void);

#endif // RECEP_SPI_H
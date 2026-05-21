/**
 * @file fil.h
 * @brief Gestion du générateur de signaux pour le suivi de fil et la transmission
 * d'ordres aux robots via modulation Tout-Ou-Rien (OOK) à 50 kHz.
 * Cible : LPC1769 (Cortex-M3)
 */

#ifndef FIL_H
#define FIL_H

#include "LPC17xx.h"
#include <stdbool.h>
#include <stdint.h>

/* ========================================================================== */
/* CONFIGURATION DES BROCHES (MAPPING GPIO)                  */
/* ========================================================================== */
// Ce mapping utilise le PORT 0 et évite toutes les broches exclues du projet.
// Rappel : Les entrées du TC4427 doivent avoir des résistances de pull-up/down.

#define PIN_N_PMOS    (1 << 4)  /**< P0.4 : Entrée INA Driver 1 -> Grille P-MOS Nord */
#define PIN_N_NMOS    (1 << 5)  /**< P0.5 : Entrée INB Driver 1 -> Grille N-MOS Nord */
#define PIN_S_PMOS    (1 << 6)  /**< P0.6 : Entrée INA Driver 2 -> Grille P-MOS Sud  */
#define PIN_S_NMOS    (1 << 7)  /**< P0.7 : Entrée INB Driver 2 -> Grille N-MOS Sud  */
#define PIN_DEBUG     (1 << 8)  /**< P0.8 : Sortie de synchronisation pour simulation/oscillo */

/* ========================================================================== */
/* TYPES DE MESSAGES (ORDRES)                         */
/* ========================================================================== */

#define CMD_TYPE_VITESSE          0x00  /**< Message 000 : Vitesse maximale en % */
#define CMD_TYPE_CHARGE_SUD       0x01  /**< Message 001 : Branche SUD pour Chargement */
#define CMD_TYPE_CHARGE_NORD      0x02  /**< Message 010 : Branche NORD pour Chargement */
#define CMD_TYPE_DECHARGE_SUD     0x03  /**< Message 011 : Branche SUD pour Déchargement */
#define CMD_TYPE_DECHARGE_NORD    0x04  /**< Message 100 : Branche NORD pour Déchargement */

/* Identifiants d'affichage (Paramètre LL sur 2 bits) */
#define AFFICHAGE_LIVREUR_A       0x00  /**< Code 00 pour afficher 'A' */
#define AFFICHAGE_LIVREUR_B       0x01  /**< Code 01 pour afficher 'B' */
#define AFFICHAGE_LIVREUR_C       0x02  /**< Code 10 pour afficher 'C' */
#define AFFICHAGE_LIVREUR_D       0x03  /**< Code 11 pour afficher 'D' */

/* ========================================================================== */
/* FONCTIONS PUBLIQUES                            */
/* ========================================================================== */

/**
 * @brief Initialise le matériel requis pour le générateur de signal.
 * - Configure les broches GPIO P0.4 à P0.8 en sortie.
 * - Met les ponts de MOSFETs en haute impédance (Hi-Z).
 * - Initialise le TIMER0 pour générer des interruptions toutes les 10 µs.
 */
void Generator_Init(void);

/**
 * @brief Envoie un ordre de modification de vitesse (Type 000).
 * @param robot_id Id du robot destinataire (4 bits : 0 à 15).
 * @param vitesse_pourcent Vitesse souhaitée en % (7 bits : 0 à 100).
 */
void Generator_Cmd_Vitesse(uint8_t robot_id, uint8_t vitesse_pourcent);

/**
 * @brief Envoie un ordre de déplacement vers une station / un poste ouvrier.
 * Concerne les types de messages 001, 010, 011 et 100.
 * * @param type_msg Type de message (CMD_TYPE_CHARGE_SUD, CMD_TYPE_CHARGE_NORD, etc.).
 * @param robot_id Id du robot destinataire (4 bits : 0 à 15).
 * @param affichage_LL Lettre à afficher (AFFICHAGE_LIVREUR_A, B, C ou D).
 * @param poste_yy Numéro de poste ouvrier ciblé (5 bits : 0 à 31).
 */
void Generator_Cmd_Station(uint8_t type_msg, uint8_t robot_id, uint8_t affichage_LL, uint8_t poste_yy);

#endif /* FIL_H */
#ifndef FIL_H
#define FIL_H

#include "LPC17xx.h"
#include <stdbool.h>
#include <stdint.h>

#define PIN_N_PMOS    (1 << 4)  /**< P0.4 : Entree INA Driver 1 -> Grille P-MOS Nord */
#define PIN_N_NMOS    (1 << 5)  /**< P0.5 : Entree INB Driver 1 -> Grille N-MOS Nord */
#define PIN_S_PMOS    (1 << 6)  /**< P0.6 : Entree INA Driver 2 -> Grille P-MOS Sud  */
#define PIN_S_NMOS    (1 << 7)  /**< P0.7 : Entree INB Driver 2 -> Grille N-MOS Sud  */
#define PIN_DEBUG     (1 << 8)  /**< P0.8 : Sortie de synchronisation pour simulation/oscillo */

#define CMD_TYPE_VITESSE          0x00  /**< Message 000 : Vitesse maximale en % */
#define CMD_TYPE_CHARGE_SUD       0x01  /**< Message 001 : Branche SUD pour Chargement */
#define CMD_TYPE_CHARGE_NORD      0x02  /**< Message 010 : Branche NORD pour Chargement */
#define CMD_TYPE_DECHARGE_SUD     0x03  /**< Message 011 : Branche SUD pour D?chargement */
#define CMD_TYPE_DECHARGE_NORD    0x04  /**< Message 100 : Branche NORD pour D?chargement */

#define AFFICHAGE_LIVREUR_A       0x00  /**< Code 00 pour afficher 'A' */
#define AFFICHAGE_LIVREUR_B       0x01  /**< Code 01 pour afficher 'B' */
#define AFFICHAGE_LIVREUR_C       0x02  /**< Code 10 pour afficher 'C' */
#define AFFICHAGE_LIVREUR_D       0x03  /**< Code 11 pour afficher 'D' */
void Generator_Init(void);
void Generator_Cmd_Vitesse(uint8_t robot_id, uint8_t vitesse_pourcent);
void Generator_Cmd_Station(uint8_t type_msg, uint8_t robot_id, uint8_t affichage_LL, uint8_t poste_yy);

#endif /* FIL_H */
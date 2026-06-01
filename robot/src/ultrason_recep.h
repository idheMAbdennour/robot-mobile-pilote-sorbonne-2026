/**
 * @file ultrason_recep.h
 * @brief Réception et décodage de l'enveloppe ultrason émise par les postes.
 *
 * Le poste émet (cf. poste/.../EmetteurUltrason.c) une enveloppe de 2 impulsions :
 *
 *      ___              ___
 *     |   |            |   |
 *  ___|   |____________|   |___
 *      P1      ESPACE    P2
 *
 *   - largeur d'impulsion P1/P2 => CÔTÉ :  200us = 'G' , 300us = 'D'
 *   - largeur de l'ESPACE      => N° POSTE : espace = 500 + numPoste*200 (us)
 *
 * Décodage = mesure des durées (front montant/descendant) via le Timer2 libre
 * (10 us/tick, déjà initialisé par le capteur inductif). Le décodage tourne
 * dans l'ISR EINT3 ; le main consomme le résultat avec ultrason_recep_lire().
 *
 * ATTENTION MATÉRIEL : sur LPC176x, seuls les ports P0 et P2 génèrent des
 * interruptions GPIO. P1.21 NE PEUT PAS être utilisé ici. Câbler le signal
 * sur un pin libre P0 ou P2 (par défaut P2.5, voir ULTRASON_RECEP_* dans le .c).
 */

#ifndef ULTRASON_RECEP_H
#define ULTRASON_RECEP_H

#include <stdint.h>

/**
 * @brief Configure le pin de réception (GPIO entrée + interruption 2 fronts).
 *        Suppose le Timer2 déjà démarré (init_capteur_inductif le fait).
 */
void init_ultrason_recep(void);

/**
 * @brief Routine d'interruption ultrason. À APPELER depuis EINT3_IRQHandler
 *        (interruptions.c), au même titre que les autres *_interrupt_routine().
 */
void ultrason_recep_interrupt_routine(void);

/**
 * @brief Garde-fou anti-blocage. À appeler à 50 Hz dans la boucle coopérative.
 *        Réarme la machine d'état si une trame partielle reste figée.
 */
void ultrason_recep_tick(void);

/**
 * @brief Récupère la dernière trame ultrason décodée.
 * @param num_poste [out] numéro du poste détecté (0..).
 * @param cote      [out] 'G' ou 'D' (côté de l'antenne émettrice).
 * @return 1 si une NOUVELLE trame valide est disponible (et la consomme),
 *         0 sinon.
 */
uint8_t ultrason_recep_lire(uint8_t *num_poste, char *cote);

/* --------------------------------------------------------------------------
 * COEUR DE DÉCODAGE PUR (indépendant du matériel).
 * Normalement appelé en interne par l'ISR / le tick. Exposé pour permettre
 * un test sur PC (voir test_ultrason.c, compilé avec -DHOST_TEST).
 * ------------------------------------------------------------------------ */

/** @brief Traite un front. niveau=1 (montant) / 0 (descendant), now en ticks Timer2. */
void ultrason_decode_edge(uint8_t niveau, uint32_t now_ticks);

/** @brief Réarme la machine d'état si une trame partielle reste figée. */
void ultrason_decode_timeout(uint32_t now_ticks);

#endif /* ULTRASON_RECEP_H */
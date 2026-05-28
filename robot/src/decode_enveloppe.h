#ifndef DECODE_ENVELOPPE_H
#define DECODE_ENVELOPPE_H

#include <stdint.h>

/**
 * @brief Traduit la période mesurée de l'enveloppe en message ou commande.
 * Modifie potentiellement le robotState et les modes de debugs.
 */
void decode_enveloppe_commande(void);

#endif // DECODE_ENVELOPPE_H
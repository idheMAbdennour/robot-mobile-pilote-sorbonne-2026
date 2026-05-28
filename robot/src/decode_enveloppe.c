#include "decode_enveloppe.h"

// Le décodage réel des symboles d'enveloppe n'est pas dans ce module.
// Ce stub reçoit la période mesurée en microsecondes et doit
// appeler le décodeur/maillon de traitement dédié (externe).
// Pour l'instant on expose simplement le point d'entrée.
void decode_enveloppe_commande(uint16_t period_us)
{
    // TODO: Appeler la fonction de décodage réelle qui reconstruit
    // la trame à partir de la série de périodes mesurées.
    (void)period_us;
}

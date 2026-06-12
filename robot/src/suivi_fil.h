#ifndef SUIVI_FIL_H
#define SUIVI_FIL_H

#include <stdint.h>

typedef enum {
    TARGET_NORD,
    TARGET_SUD
} TargetWire_t;

typedef enum {
    TRACKING_STATE_ENTRELACEMENT,
    TRACKING_STATE_NORD,
    TRACKING_STATE_SUD
} TrackingState_t;

// Initialiser la machine à état
void init_suivi_fil(void);

// Appelé quand un symbole (NORD ou SUD) est décodé pour stocker la distance correspondante
void suivi_fil_update_distances(float dist_nord_or_sud_av, float dist_nord_or_sud_ar, uint8_t is_nord);

// Mise à jour de la machine à état (appelée régulièrement ou après udpate_distances)
void suivi_fil_update_state_machine(void);

// Force l'état d'entrelacement (ex: demandé par DTMF)
void suivi_fil_force_entrelacement(void);

// Change la cible (ex: commandé par l'aiguillage)
void suivi_fil_set_target(TargetWire_t target);

// Récupère l'erreur courante pour le PID, en fonction de l'état actuel et de la cible
// Retourne vrai(1) si l'erreur a pu être calculée, faux(0) sinon.
uint8_t suivi_fil_get_active_wire_error(float *err_av, float *err_ar);

#endif // SUIVI_FIL_H

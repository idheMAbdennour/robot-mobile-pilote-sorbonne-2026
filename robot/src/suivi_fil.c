#include "suivi_fil.h"
#include <math.h>

static float distance_nord_av = 0.0f;
static float distance_nord_ar = 0.0f;
static float distance_sud_av = 0.0f;
static float distance_sud_ar = 0.0f;

static TrackingState_t current_state = TRACKING_STATE_ENTRELACEMENT;
static TargetWire_t target_wire = TARGET_NORD;

#define SEUIL_CONNEXION 0.1f

void init_suivi_fil(void) {
    current_state = TRACKING_STATE_ENTRELACEMENT;
    target_wire = TARGET_NORD;
}

void suivi_fil_update_distances(float dist_av, float dist_ar, uint8_t is_nord) {
    if (is_nord) {
        distance_nord_av = dist_av;
        distance_nord_ar = dist_ar;
    } else {
        distance_sud_av = dist_av;
        distance_sud_ar = dist_ar;
    }
    suivi_fil_update_state_machine();
}

void suivi_fil_update_state_machine(void) {
    // Calcul d'une grandeur de "faiblesse" basée sur les distances (plus la distance
    // absolue cumulée est grande, plus le signal est supposé "faible").
    float mag_nord = fabsf(distance_nord_av) + fabsf(distance_nord_ar);
    float mag_sud = fabsf(distance_sud_av) + fabsf(distance_sud_ar);

    if (current_state == TRACKING_STATE_ENTRELACEMENT) {
        if (target_wire == TARGET_NORD) {
            // Passer sur NORD si le signal NORD est plus "fort" (distance plus faible)
            // que le signal SUD, et que SUD est suffisamment "faible" (distance élevée).
            // Le réglage du seuil est à ajuster en fonction des essais réels.
            if (mag_sud > mag_nord && mag_sud > SEUIL_CONNEXION) {
                current_state = TRACKING_STATE_NORD;
            }
        } else if (target_wire == TARGET_SUD) {
            if (mag_nord > mag_sud && mag_nord > SEUIL_CONNEXION) {
                current_state = TRACKING_STATE_SUD;
            }
        }
    }
}

void suivi_fil_force_entrelacement(void) {
    current_state = TRACKING_STATE_ENTRELACEMENT;
}

void suivi_fil_set_target(TargetWire_t target) {
    target_wire = target;
}

uint8_t suivi_fil_get_active_wire_error(float *err_av, float *err_ar) {
    if (current_state == TRACKING_STATE_ENTRELACEMENT) {
        // En entrelacement, on renvoie l'erreur du fil ciblé.
        if (target_wire == TARGET_NORD) {
            if (err_av) *err_av = distance_nord_av;
            if (err_ar) *err_ar = distance_nord_ar;
        } else {
            if (err_av) *err_av = distance_sud_av;
            if (err_ar) *err_ar = distance_sud_ar;
        }
        return 1;
    } else if (current_state == TRACKING_STATE_NORD) {
        if (err_av) *err_av = distance_nord_av;
        if (err_ar) *err_ar = distance_nord_ar;
        return 1;
    } else if (current_state == TRACKING_STATE_SUD) {
        if (err_av) *err_av = distance_sud_av;
        if (err_ar) *err_ar = distance_sud_ar;
        return 1;
    }

    return 0;
}

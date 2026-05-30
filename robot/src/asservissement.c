/**
 * @file asservissement.c
 * @brief Fichier du module d'asservissement.
 */

#include "asservissement.h"
#include <math.h>
#include "asservissement.h"
#include "robot_state.h"
#include "moteur.h"

/* ==========================================================================
 * DÉFINITION DES GAINS (Globales pour modification via UART)
 * ========================================================================== */
float asserv_Kp_y = 10.0f;       // Gain pour l'écart latéral (à régler)
float asserv_Kp_alpha = 5.0f;      // Gain pour l'erreur angulaire (à régler)
float asserv_K_corr_y = 0.5f;    // Gain de confiance capteur vs odométrie pour y
float asserv_K_corr_alpha = 0.5f;  // Gain de confiance capteur vs odométrie pour alpha

/* ==========================================================================
 * ÉTAT INTERNE DE L'ODOMÉTRIE RELATIVE
 * ========================================================================== */
typedef struct {
    float y;              // erreur latérale estimée en m
    float alpha;            // angle estimé en rad
    uint8_t valid;        // 1 si l'état est fiable
} FilOdoState;

static FilOdoState current_state = {0.0f, 0.0f, 0};

/* ==========================================================================
 * FONCTION PRINCIPALE À 50Hz
 * ========================================================================== */
void asservissement_update(void) {
    // Voir le ASSERVISSEMENT_README.md pour plus de détails
    // en gros il faut lire les encodeurs et les capteurs et faire des calculs
    // pour corriger la trajectoire du robot.

    changer_pwm_moteurs((uint8_t)v_commande, (uint8_t)v_commande);
}

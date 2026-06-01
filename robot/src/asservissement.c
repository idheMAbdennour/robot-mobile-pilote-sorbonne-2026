/**
 * @file asservissement.c
 * @brief Asservissement du robot filoguidé (LPC1769, GR10).
 *
 * Suit pas à pas ASSERVISSEMENT_README.md :
 *   1. Prédiction odométrique relative (roues)
 *   2. Fusion capteur inductif (recalage)
 *   3. Loi de commande P : omega_cmd = -Kp_y*y - Kp_alpha*alpha
 *   4. Mixage différentiel, rampe sur la consigne, saturations (pas de marche
 *      arrière, jamais > 100 %)
 *
 * Le coeur de calcul est PUR (aucun registre / aucun pin) : il dialogue
 * uniquement via robot_state.h, donc il est testable au simulateur Keil.
 *
 * Ce qui reste à CALIBRER sur le stand est marqué  [A CALIBRER] / [A AFFINER].
 */

#include "asservissement.h"
#include <math.h>
#include "robot_state.h"
#include "moteur.h"

/* ==========================================================================
 * 1) GAINS (extern -> réglables par UART)  — valeurs de départ
 * ========================================================================== */

/* Loi de commande (README §5). Unités libres : ces gains se règlent au banc.
 * Point de départ raisonnable ; à affiner. */
float asserv_Kp_y     = 10.0f;   /* gain écart latéral  [A AFFINER] */
float asserv_Kp_alpha = 5.0f;    /* gain erreur angle   [A AFFINER] */

/* Fusion capteur/odométrie (README §4), entre 0 et 1. */
float asserv_K_corr_y     = 0.5f;
float asserv_K_corr_alpha = 0.5f;

/* ==========================================================================
 * 2) PARAMÈTRES FIXES
 * ========================================================================== */

/* Entraxe e (m). On le dérive du define partagé dans robot_state.h pour rester
 * synchro avec le reste du projet (120 mm de départ -> [A CALIBRER]). */
#define ENTRAXE_E_M     ((float)ROBOT_WHEEL_DISTANCE_MM / 1000.0f)

/* Plage de consigne autorisée quand le robot ROULE (CdC p.8 : 25..80 %).
 * La valeur 0 reste un cas particulier = ARRÊT (le robot doit pouvoir
 * s'arrêter), elle n'est donc pas ramenée à 25 %. */
#define VMOY_MIN_PCT    25.0f
#define VMOY_MAX_PCT    80.0f

/* Rampe de vitesse : variation max de la base PWM par tick (50 Hz).
 * 2 %/tick -> ~0.5 s pour passer de 0 à 50 %. README §5 : "v_cmd lissée". */
#define RAMP_STEP_PCT   2.0f      /* [A AFFINER] */

/* Saturation de la part différentielle (en points de PWM) : garde de la marge
 * pour que la base reste exprimée. Le mixage final est de toute façon borné
 * à [0,100] par roue (README "Sécurités finales"). */
#define TURN_MAX_PCT    40.0f     /* [A AFFINER] */

/* Bornes PWM matérielles. */
#define PWM_MIN_PCT     0.0f
#define PWM_MAX_PCT     100.0f

/* Garde-fou d'odométrie : on n'utilise la prédiction roues que si dt>0.
 * Tant que le module odométrie ne remplit pas WheelDelta (set_wheel_delta
 * jamais appelé pour l'instant), dt vaut 0 -> on saute proprement la
 * prédiction et on roule en ligne sur la consigne rampée. */

/* ==========================================================================
 * 3) ÉTAT INTERNE PERSISTANT
 * ========================================================================== */

/* Estimée fusionnée de l'état (README §1). Initialisée à zéro -> au démarrage
 * le robot se croit aligné et collé au fil ; base=0 -> il NE BOUGE PAS tant
 * qu'aucune consigne n'est reçue (sécurité mise sous tension, CdC p.10). */
static float est_y     = 0.0f;   /* écart latéral estimé [m]   */
static float est_alpha = 0.0f;   /* erreur angulaire estimée [rad] */

static float base_pwm  = 0.0f;   /* sortie de la rampe de vitesse [%] */

/* ==========================================================================
 * 4) OUTILS
 * ========================================================================== */

static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* Conversion sûre float -> uint8_t pour le PWM : on borne AVANT le cast pour
 * éviter tout repliement (un négatif casté en uint8_t donnerait 255 !). */
static uint8_t pwm_to_u8(float pwm)
{
    pwm = clampf(pwm, PWM_MIN_PCT, PWM_MAX_PCT);
    return (uint8_t)(pwm + 0.5f);   /* arrondi */
}

/* ==========================================================================
 * 5) BOUCLE 50 Hz
 * ========================================================================== */

void asservissement_update(void)
{
    /* ---- (0) Consigne de vitesse linéaire imposée par la centrale (%) ------
     * Remplie par decode_enveloppe (trame type 000). 0 = arrêt. */
    int32_t consigne_i = get_vitesse_centrale();
    float   consigne   = (float)consigne_i;

    /* Cible de base : 0 si arrêt, sinon bornée à la plage de roulage. */
    float base_cible;
    if (consigne <= 0.0f) {
        base_cible = 0.0f;
    } else {
        base_cible = clampf(consigne, VMOY_MIN_PCT, VMOY_MAX_PCT);
    }

    /* ---- (1) Prédiction odométrique (README §3) ---------------------------*/
    WheelDelta d;
    get_wheel_delta(&d);

    float y_pred     = est_y;
    float alpha_pred = est_alpha;

    if (d.dt > 0.0f) {
        float ddelta = 0.5f * (d.dd_d + d.dd_g);            /* avance moyenne [m] */
        float dtheta = (d.dd_d - d.dd_g) / ENTRAXE_E_M;     /* rotation [rad]     */

        /* ŷ = y + Δd·sin(α + Δθ/2) ; α̂ = α + Δθ */
        y_pred     = est_y + ddelta * sinf(est_alpha + 0.5f * dtheta);
        alpha_pred = est_alpha + dtheta;
    }

    /* ---- (2) Fusion capteur inductif (README §4) --------------------------*/
    WireMeasure m;
    get_wire_measure(&m);

    if (m.wire_valid) {
        /* recalage par filtre complémentaire, gains de confiance [0..1] */
        est_y     = y_pred     + asserv_K_corr_y     * (m.y_mes     - y_pred);
        est_alpha = alpha_pred + asserv_K_corr_alpha * (m.alpha_mes - alpha_pred);
    } else {
        /* fil perdu : on navigue à l'aveugle sur la prédiction roues */
        est_y     = y_pred;
        est_alpha = alpha_pred;
    }

    /* ---- (3) Loi de commande P (README §5) --------------------------------*/
    float omega_cmd = -(asserv_Kp_y * est_y) - (asserv_Kp_alpha * est_alpha);

    /* ---- (4) Rampe sur la base de vitesse (lissage de la consigne) --------*/
    if (base_pwm < base_cible) {
        base_pwm += RAMP_STEP_PCT;
        if (base_pwm > base_cible) base_pwm = base_cible;
    } else if (base_pwm > base_cible) {
        base_pwm -= RAMP_STEP_PCT;
        if (base_pwm < base_cible) base_pwm = base_cible;
    }

    /* ---- (5) Mixage différentiel (README §5) ------------------------------
     * v_g = v_cmd - (e/2)·omega ; v_d = v_cmd + (e/2)·omega
     * Exprimé en points de PWM ; la part virage est saturée pour préserver la
     * base, puis chaque roue est bornée à [0,100] (pas de marche arrière). */
#define OMEGA_TO_PWM 20.0f

float turn = clampf(OMEGA_TO_PWM * omega_cmd,
                    -TURN_MAX_PCT,
                    TURN_MAX_PCT);
    float pwm_g_f = base_pwm - turn;
    float pwm_d_f = base_pwm + turn;

    uint8_t pwm_g = pwm_to_u8(pwm_g_f);
    uint8_t pwm_d = pwm_to_u8(pwm_d_f);

    /* ---- (6) Application + recopie d'état pour debug / trames -------------*/
    changer_pwm_moteurs(pwm_g, pwm_d);

    set_motor_pwms((int32_t)pwm_g, (int32_t)pwm_d);
    set_vitesse_commande((int32_t)(base_pwm + 0.5f));
    /* v_moy commandée (%) et omega commandé (en milli-rad/s, entier) pour debug.
     * NB : on n'écrit PAS set_vitesse_reelle ici : la vitesse RÉELLE vient de
     * l'odométrie (m/s -> %), conversion qui appartient au module odométrie. */
    set_motor_speeds((int32_t)(base_pwm + 0.5f), (int32_t)(omega_cmd * 1000.0f));
}
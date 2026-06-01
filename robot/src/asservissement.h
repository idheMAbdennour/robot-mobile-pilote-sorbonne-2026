/**
 * @file asservissement.h
 * @brief Module d'asservissement du robot filoguidé (LPC1769, GR10).
 *
 * Implémente l'algorithme décrit dans ASSERVISSEMENT_README.md :
 *   1. Prédiction par odométrie relative       (WheelDelta : dd_g, dd_d, dt)
 *   2. Correction par capteur inductif (fusion) (WireMeasure : y_mes, alpha_mes)
 *   3. Loi de commande P sur (y, alpha)  ->  omega_cmd
 *   4. Mixage différentiel + rampe + saturations -> PWM moteurs
 *
 * Toutes les entrées/sorties passent par robot_state.h (pas d'accès direct
 * aux registres ici). La fonction est appelée à 50 Hz par main.c, APRÈS
 * l'acquittement de get_flag_50hz() : elle exécute donc UN tick et rien d'autre.
 */

#ifndef ASSERVISSEMENT_H
#define ASSERVISSEMENT_H

/* ==========================================================================
 * GAINS ET PARAMÈTRES D'ASSERVISSEMENT
 * Déclarés extern pour permettre un réglage à chaud via UART.
 * (Noms et sémantique conservés depuis la version d'origine du module.)
 * ========================================================================== */

/* Loi de commande (README §5) : omega_cmd = -Kp_y*y - Kp_alpha*alpha
 *  - asserv_Kp_y     : gain sur l'écart latéral y. Trop fort -> oscillation.
 *  - asserv_Kp_alpha : gain sur l'erreur angulaire alpha. Amortit la trajectoire.
 * Ces deux gains absorbent aussi le facteur d'échelle (m/s -> %PWM) : ils se
 * règlent sur le stand, exactement comme indiqué dans le README. */
extern float asserv_Kp_y;
extern float asserv_Kp_alpha;

/* Fusion de données (README §4) : gains de confiance capteur vs odométrie,
 * compris entre 0 et 1.
 *  - 0 = on ne croit que les roues (odométrie pure)
 *  - 1 = on ne croit que le capteur (mesure pure)
 *  - ~0.5 = filtre le bruit du capteur. */
extern float asserv_K_corr_y;
extern float asserv_K_corr_alpha;

/**
 * @brief Boucle d'asservissement complète, à appeler à 50 Hz depuis main.c.
 *
 * Enchaîne : prédiction odométrique -> fusion capteur -> loi de commande ->
 * mixage différentiel -> rampe de vitesse -> saturations -> écriture PWM.
 *
 * Lit dans robot_state :  get_vitesse_centrale(), get_wheel_delta(),
 *                          get_wire_measure().
 * Écrit dans robot_state : set_vitesse_commande(), set_motor_pwms(),
 *                          set_motor_speeds()  (pour le debug / les trames).
 * Agit sur le matériel via changer_pwm_moteurs().
 */
void asservissement_update(void);

#endif /* ASSERVISSEMENT_H */
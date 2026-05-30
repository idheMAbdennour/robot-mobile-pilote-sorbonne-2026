/**
 * @file asservissement.h
 * @brief Fichier du module d'asservissement.
 */

#ifndef ASSERVISSEMENT_H
#define ASSERVISSEMENT_H

/* ==========================================================================
 * GAINS ET PARAMÈTRES D'ASSERVISSEMENT
 * ========================================================================== */
// Ces gains sont déclarés extern pour permettre un réglage via UART
extern float asserv_Kp_y;
extern float asserv_Kp_alpha;
extern float asserv_K_corr_y;
extern float asserv_K_corr_alpha;

/**
 * @brief Fonction principale de la boucle d'asservissement, à appeler à 50Hz.
 *        Calcule les vitesses souhaitées (rampe) et les PWM moteurs.
 */
void asservissement_update(void);

#endif // ASSERVISSEMENT_H

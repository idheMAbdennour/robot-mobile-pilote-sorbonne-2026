#ifndef MOTEUR_H
#define MOTEUR_H

#include <stdint.h>

/**
 * @brief Initialise le module PWM1 pour les deux roues du robot.
 * Configure P2.0 (Roue Gauche) et P2.1 (Roue Droite) à une fréquence de 25 kHz.
 */
void Init_Moteur_PWM(void);

/**
 * @brief Modifie le rapport cyclique de la roue GAUCHE à tout moment.
 * @param pourcent Valeur entre 0 (arrêt) et 100 (vitesse max).
 */
void Changer_PWM_Gauche(uint8_t pourcent);

/**
 * @brief Modifie le rapport cyclique de la roue DROITE à tout moment.
 * @param pourcent Valeur entre 0 (arrêt) et 100 (vitesse max).
 */
void Changer_PWM_Droite(uint8_t pourcent);

/**
 * @brief Modifie simultanément la vitesse des deux roues.
 * @param pourcent_gauche Valeur entre 0 et 100 pour le moteur gauche.
 * @param pourcent_droite Valeur entre 0 et 100 pour le moteur droit.
 */
void Changer_PWM_Moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite);

#endif /* MOTEUR_H */
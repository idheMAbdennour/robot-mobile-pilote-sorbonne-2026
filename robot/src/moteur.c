#include "LPC17xx.h"
#include <stdint.h>

// On définit la valeur maximale du compteur pour une période de 40µs (à 100 MHz)
#define PWM_PERIOD_TICKS 4000 

/**
 * @brief Initialise le module PWM1 pour les deux roues du robot [cite: 174]
 * Utilise les broches P2.0 (PWM1.1) pour la roue gauche et P2.1 (PWM1.2) pour la roue droite.
 */
void Init_Moteur_PWM(void) {
    // 1. Activation de l'alimentation du bloc PWM1
    LPC_SC->PCONP |= (1 << 6);
    
    // 2. Configuration de l'horloge du PWM1 (PCLK_PWM1 = CCLK = 100 MHz)
    LPC_SC->PCLKSEL0 &= ~(3 << 12);
    LPC_SC->PCLKSEL0 |= (1 << 12); 
    
    // 3. Configuration des broches P2.0 et P2.1 en mode PWM1.1 et PWM1.2
    LPC_PINCON->PINSEL4 &= ~0xF;   // Reset des fonctionnalités de P2.0 et P2.1
    LPC_PINCON->PINSEL4 |= 0x5;    // Sélectionne la fonction "PWM" (01) pour ces deux broches

    // 4. Configuration du compteur (Pas de division d'horloge)
    LPC_PWM1->PR = 0;              
    
    // 5. Définition de la période globale (< 50µs pour le silence )
    LPC_PWM1->MR0 = PWM_PERIOD_TICKS; // 4000 ticks = 40 µs (Fréquence de 25 kHz)
    
    // 6. Extinction initiale des moteurs (Rapport cyclique à 0%) 
    LPC_PWM1->MR1 = 0;             // Canal 1 (Roue Gauche)
    LPC_PWM1->MR2 = 0;             // Canal 2 (Roue Droite)
    
    // 7. Demande de prise en compte immédiate des valeurs (Latch Enable)
    LPC_PWM1->LER = (1 << 0) | (1 << 1) | (1 << 2);
    
    // 8. Activation des sorties PWM1.1 et PWM1.2 en mode simple rampe
    LPC_PWM1->PCR = (1 << 9) | (1 << 10);
    
    // 9. Démarrage du compteur principal et activation du mode PWM
    LPC_PWM1->TCR = (1 << 0) | (1 << 3);
}

/**
 * @brief Modifie le rapport cyclique de la roue GAUCHE à tout moment
 * @param pourcent Valeur entre 0 (arrêt) et 100 (vitesse max)
 */
void Changer_PWM_Gauche(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;
    
    // Calcul de la correspondance en ticks pour le registre de comparaison
    LPC_PWM1->MR1 = (PWM_PERIOD_TICKS * pourcent) / 100;
    
    // CRUCIAL : On active le "Latch" pour que la modification soit appliquée 
    // proprement au début du prochain cycle PWM, sans créer de glitch électrique.
    LPC_PWM1->LER |= (1 << 1);
}

/**
 * @brief Modifie le rapport cyclique de la roue DROITE à tout moment
 * @param pourcent Valeur entre 0 (arrêt) et 100 (vitesse max)
 */
void Changer_PWM_Droite(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;
    
    // Calcul de la correspondance en ticks
    LPC_PWM1->MR2 = (PWM_PERIOD_TICKS * pourcent) / 100;
    
    // Activation du Latch pour le canal 2
    LPC_PWM1->LER |= (1 << 2);
}

/**
 * @brief Modifie les deux roues simultanément (Pratique pour ton asservissement)
 */
void Changer_PWM_Moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite) {
    if (pourcent_gauche > 100) pourcent_gauche = 100;
    if (pourcent_droite > 100) pourcent_droite = 100;
    
    LPC_PWM1->MR1 = (PWM_PERIOD_TICKS * pourcent_gauche) / 100;
    LPC_PWM1->MR2 = (PWM_PERIOD_TICKS * pourcent_droite) / 100;
    
    // On valide les deux canaux en même temps
    LPC_PWM1->LER |= (1 << 1) | (1 << 2);
}
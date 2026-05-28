#include "LPC17xx.h"
#include "moteur.h"

/**
 * @brief Attente logicielle simple pour le test (environ 1 seconde à 100MHz)
 */
void Delay_ms(uint32_t ms) {
    // Boucle d'attente brute (ajustable selon le comportement attendu)
    volatile uint32_t count;
    for (uint32_t i = 0; i < ms; i++) {
        for (count = 0; count < 12000; count++) {
            __NOP();
        }
    }
}

int main(void) {
    // 1. Initialisation du cœur et des horloges de la puce (100 MHz)
    SystemInit();
    
    // 2. Initialisation de notre module de commande moteur
    Init_Moteur_PWM();
    
    // Tableau des paliers de puissance à tester (en %)
    uint8_t paliers_vitesse[] = {0, 25, 50, 75, 100, 75, 50, 25};
    uint8_t nb_paliers = sizeof(paliers_vitesse) / sizeof(paliers_vitesse[0]);
    uint8_t index = 0;

    while (1) {
        // On applique le palier de vitesse actuel aux deux moteurs
        Changer_PWM_Moteurs(paliers_vitesse[index], paliers_vitesse[index]);
        
        // On attend 2 secondes sur ce palier pour observer le comportement
        Delay_ms(2000);
        
        // Passage au palier suivant (et retour à 0 à la fin du tableau)
        index++;
        if (index >= nb_paliers) {
            index = 0;
        }
    }
}
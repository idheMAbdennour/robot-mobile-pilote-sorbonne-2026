
#include "LPC17xx.h"
#include "moteur.h"

/**
 * @brief Attente logicielle simple pour le test (environ 1 seconde � 100MHz)
 */
void Delay_ms(uint32_t ms) {
    // Boucle d'attente brute (ajustable selon le comportement attendu)
    volatile uint32_t count;
    for (uint32_t i = 0; i < ms; i++) {
        for (count = 0; count < 12000; count++) {
            __NOP();
        }
    }
/*
#include "status.h"
#include "dtmf.h"
#include "emissionIR.h"

int main() {
	// Initialisations Status & DTMF
	// initLedChangementStatus();
	// init_dtmf();

	// Initialisations Infrarouge
	init_PWM_IR();          // Lance la porteuse 38kHz sur P1.22 (en GPIO désactivé par défaut)
	init_Timer_Enveloppe(250); // Lance le Timer 0 à 250us (un temps 't')



	// TESTS

	// mainTestStatusLED(); // Décommenter pour tester le module status
	mainTestEmissionIR(); // Décommenter pour tester la préparation d'une trame IR (sans émission réelle)

	while(1) {
		// Boucle infinie, tout est géré par les interruptions EINT3 (DTMF)
		// et TIMER0 (Emission IR) en tâche de fond !
	}
}
/*
int main(void) {
    // 1. Initialisation du c�ur et des horloges de la puce (100 MHz)
    SystemInit();
    
    // 2. Initialisation de notre module de commande moteur
    Init_Moteur_PWM();
    
    // Tableau des paliers de puissance � tester (en %)
    uint8_t paliers_vitesse[] = {0, 25, 50, 75, 100, 75, 50, 25};
    uint8_t nb_paliers = sizeof(paliers_vitesse) / sizeof(paliers_vitesse[0]);
    uint8_t index = 0;

    while (1) {
        // On applique le palier de vitesse actuel aux deux moteurs
        Changer_PWM_Moteurs(paliers_vitesse[index], paliers_vitesse[index]);
        
        // On attend 2 secondes sur ce palier pour observer le comportement
        Delay_ms(2000);
        
        // Passage au palier suivant (et retour � 0 � la fin du tableau)
        index++;
        if (index >= nb_paliers) {
            index = 0;
        }
    }
}*/
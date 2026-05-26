#include "status.h"
#include "dtmf.h"
#include "emissionIR.h"

int main() {
	// Initialisations Status & DTMF
	initLedChangementStatus();
	init_dtmf();

	// Initialisations Infrarouge
	init_PWM_IR();          // Lance la porteuse 38kHz sur P1.22 (en GPIO désactivé par défaut)
	init_Timer_Enveloppe(); // Lance le Timer 0 à 250us (un temps 't')



	// TESTS

	// mainTestStatusLED(); // Décommenter pour tester le module status
	// mainTestEmissionIR(); // Décommenter pour tester la préparation d'une trame IR (sans émission réelle)

	while(1) {
		// Boucle infinie, tout est géré par les interruptions EINT3 (DTMF)
		// et TIMER0 (Emission IR) en tâche de fond !
	}
}

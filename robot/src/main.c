#include "status.h"
#include "dtmf.h"
#include "emissionIR.h"
#include "capteurInductif.h"
#include "microswitchs.h"
#include "debug_itm.h"
#include "robotState.h"
#include "decode_enveloppe.h"
#include <stdio.h>

volatile uint8_t flag_50hz = 0;

int main() {
	// Initialisations Status & DTMF
	// initLedChangementStatus();
	// init_dtmf();

	// Initialisations Infrarouge
	init_PWM_IR();          // Lance la porteuse 38kHz sur P1.22 (en GPIO désactivé par défaut)
	init_Timer_Enveloppe(250); // Lance le Timer 0 à 250us (un temps 't')

    // Initialisations ITM et Debug
    init_debug_itm();
    init_microswitchs();
    init_capteur_inductif();

    // Configuration SysTick pour 50Hz (Période = 20ms)
    // CoreClock supposée à 100MHz (divisé par 50 pour avoir 20ms si ticks/s)
    SysTick_Config(SystemCoreClock / 50);

    // Initialisation forcée de l'état de debug pour les tests
    set_debug_itm_enabled(1);

	// TESTS
	// mainTestStatusLED(); // Décommenter pour tester le module status
	// mainTestEmissionIR(); // Décommenter pour tester la préparation d'une trame IR (sans émission réelle)

	while(1) {
		// Boucle avec condition 50Hz pour la télémétrie ITM
        if (flag_50hz) {
            flag_50hz = 0; // Acquittement du drapeau
            
            if (get_debug_itm_enabled()) {
                uint8_t sw = get_microswitch_state();
                char buffer[64];
                
                // Si l'état des switchs est 11, le choix du debug se fait par le fil
                if (sw == 3) { // '11'
                    uint8_t mode_fil = get_wire_debug_mode();
                    // Ajoutons de la logique factice à implémenter plus tard dans decode_enveloppe
                    // switch (mode_fil) ...
                }
                else if (sw == 2) { // '10' : Valeurs PWM 
                    int32_t pg, pd;
                    get_motor_pwms(&pg, &pd);
                    sprintf(buffer, "G %d D %d\r\n", (int)pg, (int)pd);
                    itm_send_string(buffer);
                }
                else if (sw == 1) { // '01' : Vitesse moyenne
                    int32_t vmoy, wang;
                    get_motor_speeds(&vmoy, &wang);
                    sprintf(buffer, "V %dcm /s\r\n", (int)vmoy);
                    itm_send_string(buffer);
                }
                else if (sw == 0) { // '00' : Vitesse angulaire
                    int32_t vmoy, wang;
                    get_motor_speeds(&vmoy, &wang);
                    sprintf(buffer, "W %ddeg/s\r\n", (int)wang);
                    itm_send_string(buffer);
                }
            }
        }
        
        // Tout le reste est géré par interruptions: 
        // EINT3 (DTMF, Capteurs Inductifs, Switchs) et TIMER0 (IR)
	}
}

#include "LPC17xx.h"
#include "./KeyPad_functions/KeyPad_lecture.h"
#include "./PostConfigs/Num_Post.h"
#include "./InterfaceCentalePoste/InterfaceCentralePoste.h"
#include "./InterfaceRobotPoste/RecepteurIR.h"
#include "./InterfaceRobotPoste/EmetteurUltrason.h"

// 1. Définition des états de notre Machine à États (State Machine)
typedef enum {
    STATE_IDLE,         // Libre : scrutation clavier, attente des interruptions
    STATE_PARSE,        // Paquet reçu de la Centrale : vérification du numéro de poste
    STATE_SEND_REPLY,   // Time-slot alloué : envoi de la réponse (strictement 6 caractères)
    STATE_BLINK         // Débogage non-bloquant des LEDs (priorité basse)
} system_state_t;

volatile system_state_t current_state = STATE_IDLE;

volatile uint32_t systick_ticks = 0;
volatile uint32_t tick_ms = 0;

void init_systick_50us(void) {
    SysTick_Config(SystemCoreClock / 20000); 
}

void SysTick_Handler(void) {
    systick_ticks++;
    if (systick_ticks % 20 == 0) {
        tick_ms++;
    }
}

uint32_t slot_timer = 0;
int should_reply = 0; 

int main(void) {
    // ------ CONFIGURATION POSTE ------ //
    init_switch();
    config_numero_poste(); 
 
    // ------ PHASE D'INITIALISATION DES COMMUNICATIONS ----- //
    init_systick_50us();
    init_uart();
    init_gpio();  
		
		// ------ INITIALISATION DU RECEPTEUR IR --------- //
		init_recepteur_ir();
	
    // ------ INIT DES LEDS DEBUG -------- //
    init_leds();
		
	// ------ INITIALISATION DE L'EMETTEUR US ------ //
    initUltrasonsPWM_Continuous();
		initUltrasonsEnvelope_GPIO();
 
    // ---- INIT_TIMER --------- //
    init_timer();
		
		while(1) {
        switch(current_state) {
            case STATE_IDLE:
								//trigger_envelope_hardware(num_post, 'G'); 
                handle_display_leds_non_blocking();
                //handle_ultrasons_envelope_non_blocking();
                if (packet_ready) {
										packet_ready = 0;
										//affichage_etat_lecture(-1); 
                    current_state = STATE_PARSE;
                }
                break;
                
            case STATE_PARSE: 
								//affichage_etat_lecture(-1); 
                slot_timer = tick_ms; 
                should_reply = Message_Traitement();
                
                if (should_reply) {
                    current_state = STATE_SEND_REPLY;
                } else {
                    current_state = STATE_IDLE; 
                }
                break;
                
            case STATE_SEND_REPLY: 
                if (tick_ms - slot_timer >= 2) { 
                    Preparation_Message();
                    should_reply = 0; 
                    while (!(LPC_UART3->LSR & (1 << 6)));
                    current_state = STATE_BLINK;
                }
                break;
                
            case STATE_BLINK: 
                current_state = STATE_IDLE;
                break;
                
            default:
                current_state = STATE_IDLE;
                break;
    }
}
		}
#include "LPC17xx.h"
#include "./Config/Config_numeroPoste.h"
#include "./InterfacePoste/SendMessagePoste.h"
#include "./InterfacePoste/InterfacePostecentraleConfig.h"
#include "./InterfacePoste/ReceiveMessagePoste.h"
#include "./ReceptionIR/RecepteurIR_Centrale.h"
#include "./EmetteurDTMF/GestionAiguillage.h"
#include "./EmetteurDTMF/EmetteurDTMF.h"


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

int main(void) {
    init_switch();
    config_numero_poste();
    setup_dynamic_network(actual_total_posts);
    init_systick_50us();
    init_uart3_centrale();
    init_leds();
    init_recepteur_ir_centrale();
		init_timer();

    
    volatile uint32_t delay;
    for(delay = 0; delay < 10000; delay++);
		dtmf_send_command(5, 'A'); 
    while(1) {
        polling_system_non_blocking();
        gestion_intersection_process();
        //dtmf_sequence_process_non_blocking();
        handle_centrale_leds_non_blocking();
    }
}
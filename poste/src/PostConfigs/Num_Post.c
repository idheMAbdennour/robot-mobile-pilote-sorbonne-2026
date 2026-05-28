#include "Num_Post.h"

volatile int num_post = 0; 
volatile int est_zone_nord = 0; 

void init_switch(void) {
    LPC_PINCON->PINSEL4 &= ~(0xFF << 0);  
    LPC_GPIO2->FIODIR   &= ~(0xF << 0); 
    LPC_PINCON->PINMODE4 |= (0xFF << 0);

    
    LPC_PINCON->PINSEL4 &= ~(3 << 8);  
    LPC_GPIO2->FIODIR   &= ~(1 << 4);  
    LPC_PINCON->PINMODE4 |= (3 << 8);  
		
	
	//TEST
		LPC_PINCON->PINSEL9 &= ~(3 << 24);    // ????? GPIO ??? P4.28
    LPC_GPIO4->FIODIR   &= ~(1 << 28);    // ???????????: ????
    LPC_PINCON->PINMODE9 &= ~(3 << 24);   // ?????: 00 -> ???????? ?????????? ???????? PULL-UP (+3.3V)
}

void config_numero_poste(void) {
    volatile uint32_t delay;
    for(delay = 0; delay < 100; delay++);  
    uint32_t current_fiopin = LPC_GPIO2->FIOPIN;
    num_post = current_fiopin & 0xF; 
    int pos = (current_fiopin >> 4) & 0x1; 
    est_zone_nord = pos;
    affichage_num(num_post, est_zone_nord);
}

void affichage_num(int cpt, int p) {
    LPC_GPIO3->FIODIR |= (1 << 26); 
    LPC_GPIO3->FIOSET = (1 << 26); 
    volatile uint32_t delay;
    while(cpt > 0) {
        LPC_GPIO3->FIOCLR = (1 << 26); 
        for(delay = 0; delay < 800000; delay++);
        LPC_GPIO3->FIOSET = (1 << 26); 
        for(delay = 0; delay < 800000; delay++);
        cpt--; 
    }
    for(delay = 0; delay < 1500000; delay++);
    if (p == 1) {
        LPC_GPIO3->FIOCLR = (1 << 26); 
        for(delay = 0; delay < 2000000; delay++);
        LPC_GPIO3->FIOSET = (1 << 26); 
        for(delay = 0; delay < 800000; delay++);
    }
}
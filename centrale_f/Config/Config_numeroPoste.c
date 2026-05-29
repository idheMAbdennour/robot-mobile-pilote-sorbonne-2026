#include "Config_numeroPoste.h"

volatile int num_post = 0;
uint32_t actual_total_posts = 0;       
uint32_t posts_list[MAX_POSTS];     
uint32_t current_post_idx = 0;

void init_switch(void) {
    LPC_PINCON->PINSEL4 &= ~(0xFF << 0);
    LPC_GPIO2->FIODIR   &= ~(0xF << 0);
    LPC_PINCON->PINMODE4 |= (0xFF << 0);
}

void config_numero_poste(void) {
    volatile uint32_t delay;
    for(delay = 0; delay < 50; delay++);
    
    int pin_values = (LPC_GPIO2->FIOPIN >> 0);
    int num = pin_values & 0xF;
    
    actual_total_posts = num;
    num_post = num; 
}


void setup_dynamic_network(uint32_t nb_postes) {
    if (nb_postes > MAX_POSTS) nb_postes = MAX_POSTS;
    if (nb_postes == 0) nb_postes = 1;

    actual_total_posts = nb_postes;

    for (uint32_t i = 0; i < actual_total_posts; i++) {
        posts_list[i] = i + 1;   
    }

    current_post_idx = 0;
}
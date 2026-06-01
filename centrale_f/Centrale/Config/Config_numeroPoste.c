#include "Config_numeroPoste.h"

volatile int num_post = 0;
uint32_t actual_total_posts = 0;  // Nb postes dans système     
uint32_t posts_list[MAX_POSTS];   // tab des postes dans système  
uint32_t current_post_idx = 0;
uint32_t actual_total_robots = 0; // Nb robots dans système

void init_switch(void) {
    // P2.0 - P2.3 - Nb postes
    // P2.4 - P.7 - Nb robots
    LPC_PINCON->PINSEL4 &= ~(0xFFFF << 0);
    LPC_GPIO2->FIODIR   &= ~(0xFF << 0);
    LPC_PINCON->PINMODE4 &= ~(0xFFFF << 0);
}

void Centrale_Read_Configuration(void) {
    volatile uint32_t delay;
    for(delay = 0; delay < 50; delay++);

    uint32_t pin_values = LPC_GPIO2->FIOPIN;
    
    uint32_t nb_postes = pin_values & 0x0F;
    uint32_t nb_robots = (pin_values >> 4) & 0x0F;

    if (nb_postes == 0)  nb_postes = 1;
    if (nb_postes > MAX_POSTS) nb_postes = MAX_POSTS;

    if (nb_robots == 0)  nb_robots = 1;
    if (nb_robots > MAX_ROBOTS) nb_robots = MAX_ROBOTS;

    actual_total_posts = nb_postes;
    num_post = nb_postes; 

    actual_total_robots = nb_robots;
    
    for (uint32_t i = 0; i < actual_total_posts; i++) {
        posts_list[i] = i + 1;   
    }

    current_post_idx = 0;
}
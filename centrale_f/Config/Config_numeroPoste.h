#ifndef CONFIG_NUMERO_POSTE_H
#define CONFIG_NUMERO_POSTE_H

#include "LPC17xx.h"

#ifndef MAX_POSTS
#define MAX_POSTS 99 
#endif

extern volatile int num_post;
extern uint32_t actual_total_posts;
extern uint32_t posts_list[MAX_POSTS];
extern uint32_t current_post_idx;

void init_switch(void);
void config_numero_poste(void);
void setup_dynamic_network(uint32_t nb_postes);

#endif // CONFIG_NUMERO_POSTE_H
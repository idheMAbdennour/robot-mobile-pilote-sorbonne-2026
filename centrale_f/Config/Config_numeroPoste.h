#ifndef CONFIG_NUMERO_POSTE_H
#define CONFIG_NUMERO_POSTE_H

#include "LPC17xx.h"

#ifndef MAX_POSTS
#define MAX_POSTS 99 
#endif

#ifndef MAX_ROBOTS
#define MAX_ROBOTS 16
#endif

extern volatile int num_post;
extern uint32_t actual_total_posts;
extern uint32_t posts_list[MAX_POSTS];
extern uint32_t current_post_idx;
extern uint32_t actual_total_robots;

void init_switch(void);
void Centrale_Read_Configuration(void);

#endif // CONFIG_NUMERO_POSTE_H
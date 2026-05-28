#ifndef NUM_POST_H
#define NUM_POST_H

#include "LPC17xx.h"

extern volatile int num_post;
extern volatile int est_zone_nord;

void init_switch(void);
void config_numero_poste(void);
void affichage_num(int cpt, int p);

#endif // NUM_POST_H
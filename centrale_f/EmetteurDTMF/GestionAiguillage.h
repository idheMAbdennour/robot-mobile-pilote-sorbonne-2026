#ifndef GESTION_AIGUILLAGE_H
#define GESTION_AIGUILLAGE_H

#include "LPC17xx.h"
#include "../InterfacePoste/ReceiveMessagePoste.h"

extern volatile uint8_t intersection_busy;
extern volatile int8_t robot_inside_intersection;
extern volatile int8_t robot_waiting_on_entrance;

void gestion_intersection_process(void);

#endif
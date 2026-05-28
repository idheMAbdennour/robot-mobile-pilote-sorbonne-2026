#ifndef RECEPTEUR_IR_H
#define RECEPTEUR_IR_H

#include "LPC17xx.h"
#include "EmetteurUltrason.h"
#include "../PostConfigs/Num_Post.h"


typedef struct {
    uint8_t id;              
    char speed_hex;         
    char status_char;         
    volatile uint8_t pending; 
} LastSeenRobot;

extern volatile LastSeenRobot last_robot;

void init_recepteur_ir(void);
void EINT3_IRQHandler(void);
void process_ir_edge(void);

#endif // RECEPTEUR_IR_H
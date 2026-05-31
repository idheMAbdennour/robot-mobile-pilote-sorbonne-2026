#ifndef RECEPTEUR_IR_CENTRALE_H
#define RECEPTEUR_IR_CENTRALE_H

#include "LPC17xx.h"
#include "../InterfacePoste/ReceiveMessagePoste.h"

#define IR_PIN_NORTH    21  // P0.21
#define IR_PIN_SOUTH    22  // P0.22

void init_recepteur_ir_centrale(void);
void process_ir_edge(uint8_t pin_num, uint8_t is_north);
void EINT3_IRQHandler(void);

#endif
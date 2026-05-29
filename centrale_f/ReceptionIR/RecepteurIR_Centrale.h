#ifndef RECEPTEUR_IR_CENTRALE_H
#define RECEPTEUR_IR_CENTRALE_H

#include "LPC17xx.h"
#include "../InterfacePoste/ReceiveMessagePoste.h"

void init_recepteur_ir_centrale(void);
void process_ir_edge_centrale(void);
void EINT3_IRQHandler(void);

#endif
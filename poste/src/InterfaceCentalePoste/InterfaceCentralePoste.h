#ifndef INTERFACE_POSTE_CENTRALE_H
#define INTERFACE_POSTE_CENTRALE_H

#include "LPC17xx.h"
extern volatile int packet_ready;

void init_uart(void);
void UART3_IRQHandler(void);
int Message_Traitement(void);
void UART3_SendChar(char c);
void Poste_SendSMS(char* msg, uint32_t taille_msg);
void Preparation_Message(void);


#endif
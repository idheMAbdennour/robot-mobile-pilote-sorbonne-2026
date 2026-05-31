#ifndef SUPERVISION_H
#define SUPERVISION_H

#include "LPC17xx.h"

#include <stdint.h>
#include "../InterfacePoste/ReceiveMessagePoste.h"
#include "../GenerateurFil/fil.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void Init_Supervision_UART0(uint32_t baudrate);
void Supervision_Push_TX_Queue(const char *str);
void Supervision_Process_TX_Non_Blocking(void);
void Supervision_Process_Incoming_Non_Blocking(void);
void Supervision_Forward_Poste_Message(uint8_t post_id, const char *raw_msg);
void Supervision_Manage_Wire_Transmission_Tick(void);

#endif
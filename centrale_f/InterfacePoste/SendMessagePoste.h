#ifndef SEND_MESSAGE_POSTE_H
#define SEND_MESSAGE_POSTE_H

#include "LPC17xx.h"
#include "../Config/Config_numeroPoste.h"

typedef enum {
    POLL_SEND_REQ,
    POLL_WAIT_RESP,
    POLL_INTER_SLOT_DELAY
} poll_substate_t;

extern uint32_t posts_list[MAX_POSTS]; 
extern volatile uint32_t inter_slot_timer;
extern poll_substate_t poll_state;
extern volatile uint32_t timeout_timer;

void UART3_SendChar(char c);
void Centrale_SendSMS(int post_number);
void Centrale_Indicator_Tx(int state);
void polling_system_non_blocking(void);

#endif
#include "LPC17xx.h"
#include "SendMessagePoste.h"
#include "ReceiveMessagePoste.h"

#define MAX_POSTS 99 

extern volatile uint32_t tick_ms;
volatile uint32_t timeout_timer = 0;         
volatile uint32_t inter_slot_timer = 0;

extern volatile int packet_ready;
extern volatile int rx_index;

extern uint32_t actual_total_posts;       
extern uint32_t posts_list[MAX_POSTS];     
extern uint32_t current_post_idx;      

poll_substate_t poll_state = POLL_SEND_REQ;


void UART3_SendChar(char c) {
    while (!(LPC_UART3->LSR & (1 << 5)));  
    LPC_UART3->THR = c;
}

void Centrale_SendSMS(int post_number) {
    int tens = post_number / 10;   
    int units = post_number % 10;  
    
    UART3_SendChar('@');
    UART3_SendChar('O');
    UART3_SendChar(tens + '0');   
    UART3_SendChar(units + '0');  
    
    UART3_SendChar(0x0A); 
    UART3_SendChar(0x0D); 
}

void Centrale_Indicator_Tx(int state) {
		/*
    if (state == 1) {
        LPC_GPIO3->FIOCLR = (1 << 25);
    } else {
        LPC_GPIO3->FIOSET = (1 << 25);
    }*/
} 

void polling_system_non_blocking(void) {
    if (actual_total_posts == 0) return;

    switch(poll_state) {
        
        case POLL_SEND_REQ: {
            uint32_t active_post = posts_list[current_post_idx];
            
            rx_index = 0;
            packet_ready = 0;
            
            Centrale_Indicator_Tx(1);    
            
            Centrale_SendSMS(active_post); 
            
            while (!(LPC_UART3->LSR & (1 << 6))); 
            
            Centrale_Indicator_Tx(0); 
            
            timeout_timer = tick_ms;     
            poll_state = POLL_WAIT_RESP;
            break;
        }
            
        case POLL_WAIT_RESP:

            if (packet_ready) {
                Traitement_Response_Post(posts_list[current_post_idx]);
                
                rx_index = 0;
                packet_ready = 0;
                
                inter_slot_timer = tick_ms; 
                poll_state = POLL_INTER_SLOT_DELAY;
            }
           
            else if ((uint32_t)(tick_ms - timeout_timer) >= 15) {
                Traitement_Timeout_Error();
                
                rx_index = 0;
                packet_ready = 0;
                
                inter_slot_timer = tick_ms; 
                poll_state = POLL_INTER_SLOT_DELAY;
            }
            break;
            
        case POLL_INTER_SLOT_DELAY:
            if ((uint32_t)(tick_ms - inter_slot_timer) >= 150) {
                current_post_idx++;
                if (current_post_idx >= actual_total_posts) {
                    current_post_idx = 0; 
                }
                poll_state = POLL_SEND_REQ;
            }
            break;
            
        default:
            poll_state = POLL_SEND_REQ;
            break;
    }
}
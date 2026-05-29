#include "ReceiveMessagePoste.h"

volatile char rx_buffer[BUFFER_SIZE];
volatile int rx_index = 0;
volatile int packet_ready = 0;

Robot_Data_t robots_db[MAX_ROBOTS];
Request_Data_t posts_requests[MAX_POSTS];

static uint32_t led_timer = 0;
static int led_blink_cycles = 0;
static int led_state = 0;

extern volatile uint32_t tick_ms;

void UART3_IRQHandler(void) {
    uint32_t iir = LPC_UART3->IIR;
    
    if ((iir & 0x1) == 0) {
        uint32_t int_id = (iir >> 1) & 0x7;
        
        if (int_id == 2 || int_id == 6) {
            char c = LPC_UART3->RBR;
            
            if (packet_ready == 0) {
                if (c == '\r' || c == '\n') {
                    if (rx_index == 4) { 
                        rx_buffer[rx_index] = '\0';
                        packet_ready = 1; 
                    } else if (rx_index > 0) {
                        rx_index = 0;
                    }
                } 
                else if (rx_index < 4) {
                    rx_buffer[rx_index++] = c;
                } else {
                    rx_index = 0;
                }
            }
        }
    }
}

void Traitement_Response_Post(uint32_t post_id) {
    //LPC_GPIO0->FIOSET = (1 << 22); 
    
    if (rx_buffer[0] == 'N' && rx_buffer[1] == 'U' && rx_buffer[2] == 'L' && rx_buffer[3] == 'L') {
        return;
    }
    
    if (rx_buffer[0] == 'R') {
        uint8_t r_id = rx_buffer[1] - '0';
        uint8_t r_speed = rx_buffer[2];
        char r_status = rx_buffer[3];
        
        if (r_id < MAX_ROBOTS) {
            robots_db[r_id].robot_id = r_id;
            robots_db[r_id].vitesse_actuelle = r_speed;
            robots_db[r_id].status = r_status;
            
            if (r_status == 'E') {
                robots_db[r_id].enlevement = 0;
            }
            else if (r_status == 'D') {
                robots_db[r_id].livraison = 0;
            }
        }
    }
    else if (rx_buffer[0] >= 'A' && rx_buffer[0] <= 'D' && rx_buffer[1] == 'P') {
        char service = rx_buffer[0];
        
        int parsed_dest = 0;
        parsed_dest = (rx_buffer[2] - '0') * 10 + (rx_buffer[3] - '0');
        
        if (post_id < MAX_POSTS) {
            posts_requests[post_id].destination_post = parsed_dest;
            posts_requests[post_id].lettre_service = service;
            posts_requests[post_id].active = 1;
        }
    }
    
    char first_char = rx_buffer[0];
    int blink_count = 0;
    
    if (first_char == 'A' || first_char == 'a') blink_count = 1;
    else if (first_char == 'B' || first_char == 'b') blink_count = 2;
    else if (first_char == 'C' || first_char == 'c') blink_count = 3;
    else if (first_char == 'D' || first_char == 'd') blink_count = 4;
    
    if (blink_count > 0) {
        led_blink_cycles = blink_count * 2; 
        led_timer = tick_ms;
        led_state = 0;
    } else {
        led_blink_cycles = 0;
        LPC_GPIO3->FIOSET = (1 << 26);
    }
}

void Traitement_Timeout_Error(void) {
    //LPC_GPIO0->FIOCLR = (1 << 22); 
    //led_blink_cycles = 0;
    //LPC_GPIO3->FIOSET = (1 << 26); 
}

void handle_centrale_leds_non_blocking(void) {
    if (led_blink_cycles <= 0) return;
    
    if ((uint32_t)(tick_ms - led_timer) >= 150) {
        led_timer = tick_ms;
        
        if (led_state == 0) {
            LPC_GPIO3->FIOCLR = (1 << 26); 
            led_state = 1;
        } else {
            LPC_GPIO3->FIOSET = (1 << 26);
            led_state = 0;
        }
        
        led_blink_cycles--;
    }
}
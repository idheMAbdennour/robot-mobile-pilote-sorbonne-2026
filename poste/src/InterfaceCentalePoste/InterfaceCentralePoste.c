#include "LPC17xx.h"
#include "InterfaceCentralePoste.h"
#include <stdio.h>
#include "../KeyPad_functions/KeyPad_lecture.h"
#include "../PostConfigs/Num_Post.h"
#include "../InterfaceRobotPoste/RecepteurIR.h"

#define BUFFER_SIZE 16

volatile char rx_buffer[BUFFER_SIZE];
volatile int rx_index = 0;
volatile int packet_ready;

void affichage_etat_lecture(int code);

void init_uart(void) {
    LPC_SC->PCONP |= (1 << 25);
    LPC_SC->PCLKSEL1 &= ~(3 << 18);
    LPC_PINCON->PINSEL0 &= ~0xF;
    LPC_PINCON->PINSEL0 |= (2 << 0) | (2 << 2);
    LPC_PINCON->PINMODE0 &= ~(3 << 2);
    LPC_UART3->LCR = (3 << 0) | (1 << 7);
    LPC_UART3->DLL = 0xA3;
    LPC_UART3->DLM = 0x00;
    LPC_UART3->LCR &= ~(1 << 7);
    LPC_UART3->FCR = (1 << 0) | (1 << 1) | (1 << 2);
    LPC_UART3->IER |= (1 << 0);
    NVIC_EnableIRQ(UART3_IRQn);
}

void UART3_IRQHandler(void) {
    uint32_t iir = LPC_UART3->IIR;
    
    if ((iir & 0x1) == 0) {
        uint32_t int_id = (iir >> 1) & 0x7;
        
        if (int_id == 2 || int_id == 6) {
            char c = LPC_UART3->RBR;
            
            if (packet_ready == 0) {
                if (c == '\r' || c == '\n') {
                    if (rx_index > 0) { 
                        rx_buffer[rx_index] = '\0'; 
                        packet_ready = 1; 
                        rx_index = 0;     
                    }
                } 
                else {
                    if (rx_index < (BUFFER_SIZE - 1)) {
                        rx_buffer[rx_index++] = c; 
                    } else {
                        rx_index = 0; 
                    }
                }
            }
        }
    }
}


int Message_Traitement(void) {
		//affichage_etat_lecture(-1); 
    int is_for_us = 0;
    int tens = rx_buffer[2] - '0';
    int units = rx_buffer[3] - '0';
    if (tens >= 0 && tens <= 9 && units >= 0 && units <= 9) {
        int target_post = (tens * 10) + units;
        if (target_post == num_post) {
						affichage_etat_lecture(-1);
            is_for_us = 1; 
        }
    }
    //rx_index = 0; 
    //packet_ready = 0;
    
    return is_for_us;
}

void UART3_SendChar(char c) {
    while (!(LPC_UART3->LSR & (1 << 5)));
    LPC_UART3->THR = c;
}

void Poste_SendSMS(char* msg, uint32_t taille_msg) {
    for (uint32_t i = 0; i < taille_msg; i++) {
        UART3_SendChar(msg[i]);
    }
		//affichage_etat_lecture(1);
}


void Preparation_Message(void) {
    char msg[8];
    if (last_robot.pending) {
        snprintf(msg, sizeof(msg), "R%d%c%c\r\n", 
                 last_robot.id, 
                 last_robot.speed_hex, 
                 last_robot.status_char);
                 
        Poste_SendSMS(msg, 6);
        last_robot.pending = 0;
    } 
    else if (msg_count > 0) {
         
        char target_tens  = msg_buffer[0].text[1]; 
        char target_units = msg_buffer[0].text[2];
        char colis        = msg_buffer[0].text[3];
        
        if (est_zone_nord == 0 && colis >= 'A' && colis <= 'D') {
            colis += 32;
        }
        
        snprintf(msg, sizeof(msg), "%cP%c%c\r\n", colis, target_tens, target_units);
        Poste_SendSMS(msg, 6);
        msg_count--;
        for (int i = 0; i < msg_count; i++) {
            msg_buffer[i] = msg_buffer[i+1]; 
        }
    } 
    else {
        
        snprintf(msg, sizeof(msg), "NULL\r\n"); 
        Poste_SendSMS(msg, 6);
    }
}
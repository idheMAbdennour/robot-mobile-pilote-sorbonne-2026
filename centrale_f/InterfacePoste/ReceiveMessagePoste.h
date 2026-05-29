#ifndef RECEIVE_MESSAGE_POSTE_H
#define RECEIVE_MESSAGE_POSTE_H

#include "LPC17xx.h"

#define BUFFER_SIZE 16
#define MAX_ROBOTS  16
#define MAX_POSTS   99

typedef struct {
    uint8_t robot_id;
    uint8_t vitesse_actuelle;
    uint8_t vitesse_voulue;
    char status;
    uint8_t enlevement;
    uint8_t livraison;
    char lettre_service;
		uint8_t at_intersection_entrance;
} Robot_Data_t;

typedef struct {
    uint8_t destination_post;
    char lettre_service;
    uint8_t active;
} Request_Data_t;

extern Robot_Data_t robots_db[MAX_ROBOTS];
extern Request_Data_t posts_requests[MAX_POSTS];

extern volatile char rx_buffer[BUFFER_SIZE];
extern volatile int rx_index;
extern volatile int packet_ready;

void UART3_IRQHandler(void);
void Traitement_Response_Post(uint32_t post_id);
void Traitement_Timeout_Error(void);
void handle_centrale_leds_non_blocking(void);

#endif
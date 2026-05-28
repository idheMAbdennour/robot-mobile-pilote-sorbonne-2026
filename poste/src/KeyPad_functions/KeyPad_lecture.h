#ifndef KEYPAD_LECTURE_H
#define KEYPAD_LECTURE_H

#include "LPC17xx.h"

typedef struct {
	char text[6];
} Message;

extern Message msg_buffer[4];
extern int msg_count;
extern int kb_count;
extern char clavier[4][4];

void affichage_etat_lecture(int code);
void handle_display_leds_non_blocking(void);
void reset_current_input(void);
void process_key(uint8_t key);
void init_gpio(void);
void init_timer(void);
void TIMER0_IRQHandler(void);
void init_leds(void);

#endif // KEYPAD_LECTURE_H
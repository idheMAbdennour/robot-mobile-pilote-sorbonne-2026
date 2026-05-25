#ifndef EMISSION_IR_H
#define EMISSION_IR_H

#include <stdint.h>
#include "LPC17xx.h"

#define MAX_SEQ_LENGTH 50

extern uint8_t ir_sequence[MAX_SEQ_LENGTH];
extern volatile uint8_t seq_index;
extern volatile uint8_t seq_length;

extern volatile uint8_t frame_counter;

// Prototypes IR
void init_PWM_IR(void);
void init_Timer_Enveloppe(void);
void preparer_trame(uint8_t id, uint8_t vitesse, uint8_t status);

#endif // EMISSION_IR_H

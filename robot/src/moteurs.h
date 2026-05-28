#ifndef MOTEURS_H
#define MOTEURS_H

#include <stdint.h>

#define PIN_MOT_SW1 (1 << 11) // P0.11
#define PIN_MOT_SW2 (1 << 12) // P0.12

void init_moteurs_debug(void);
void moteurs_interrupt_routine(void);
void moteurs_receive_wire_command(uint8_t wire_code);
void debug_moteurs_send_frame(void);

#endif // MOTEURS_H

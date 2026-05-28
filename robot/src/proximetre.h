#ifndef PROXIMETRE_H
#define PROXIMETRE_H

#include <stdint.h>

#define PIN_PROX_SW1 (1 << 12) // P0.12
#define PIN_PROX_SW2 (1 << 13) // P0.13

#define NUM_PROXI_MEASUREMENTS 72

void init_proximetre(void);
void proximetre_interrupt_routine(void);
void debug_proximetre_send_frame(void);

#endif // PROXIMETRE_H

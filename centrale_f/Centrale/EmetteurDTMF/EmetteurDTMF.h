#ifndef EMETTEUR_DTMF_H
#define EMETTEUR_DTMF_H

#include "LPC17xx.h"
#include <math.h>

void init_timer(void);
void dtmf_set_char(char c);
void dtmf_stop_sound(void);
void dtmf_send_command(uint8_t robot_id, char action);
void dtmf_sequence_process_non_blocking(void);

#endif
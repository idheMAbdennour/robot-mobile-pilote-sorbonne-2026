#ifndef PROXIMETRE_H
#define PROXIMETRE_H
 
#include <stdint.h>
 
#define PIN_PROX_SW1 (1 << 12) 
#define PIN_PROX_SW2 (1 << 13) 
#define NUM_PROXI_MEASUREMENTS  25
 
void init_proximetre(void);                  
void proximetre_interrupt_routine(void);     
void proximetre_timer_interrupt_routine(void); 
void proximetre_run_balayage(void);          
void get_local_proxi_buffer(int32_t *out);      
int  get_proxi_count(void);                  
void debug_proximetre_send_frame(void);      
 
#endif
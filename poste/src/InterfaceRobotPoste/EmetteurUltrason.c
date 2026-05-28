#include "LPC17xx.h"
#include "EmetteurUltrason.h"

extern volatile uint32_t systick_ticks; 

typedef enum {
    US_IDLE,
    US_PULSE_1,
    US_SPACE,
    US_PULSE_2
} us_state_t;

static us_state_t us_current_state = US_IDLE;
static uint32_t us_timer = 0;
static uint32_t us_pulse_ticks = 0;
static uint32_t us_space_ticks = 0;

void initUltrasonsPWM_Continuous(void) {
    LPC_SC->PCONP |= (1 << 6); 
    
    LPC_SC->PCLKSEL0 &= ~(3 << 12); 
    LPC_SC->PCLKSEL0 |= (1 << 12);
    
    LPC_PINCON->PINSEL3 |= (1 << 15);
    LPC_PINCON->PINSEL3 &= ~(1 << 14);
    
    LPC_PWM1->PR = 0;             
    LPC_PWM1->MCR |= (1 << 1);    
    
    LPC_PWM1->MR0 = 2500;       
    LPC_PWM1->MR4 = 1250;     
    
    LPC_PWM1->LER |= (1 << 0) | (1 << 4); 
    LPC_PWM1->PCR |= (1 << 12);           
    LPC_PWM1->TCR = (1 << 0) | (1 << 3);
}

void initUltrasonsEnvelope_GPIO(void) {
    LPC_PINCON->PINSEL3 &= ~(3 << 6);
    LPC_GPIO1->FIODIR |= (1 << 19);
    LPC_GPIO1->FIOCLR = (1 << 19);
}

void trigger_envelope_hardware(int numPoste, char coter) {
    if (us_current_state != US_IDLE) return; 

    uint32_t pulse_us = (coter == 'G') ? 200 : 300;
    uint32_t space_us = 500 + (numPoste * 200);

    us_pulse_ticks = pulse_us / 50; 
    us_space_ticks = space_us / 50; 

    LPC_GPIO1->FIOSET = (1 << 19);
    us_timer = systick_ticks;      
    us_current_state = US_PULSE_1;
}

void handle_ultrasons_envelope_non_blocking(void) {
    if (us_current_state == US_IDLE) return;

    switch (us_current_state) {
        case US_PULSE_1:
            if ((uint32_t)(systick_ticks - us_timer) >= us_pulse_ticks) {
                LPC_GPIO1->FIOCLR = (1 << 19);
                us_timer = systick_ticks;
                us_current_state = US_SPACE;
            }
            break;

        case US_SPACE:
            if ((uint32_t)(systick_ticks - us_timer) >= us_space_ticks) {
                LPC_GPIO1->FIOSET = (1 << 19);
                us_timer = systick_ticks;
                us_current_state = US_PULSE_2;
            }
            break;

        case US_PULSE_2:
            if ((uint32_t)(systick_ticks - us_timer) >= us_pulse_ticks) {
                LPC_GPIO1->FIOCLR = (1 << 19);
                us_current_state = US_IDLE;    
            }
            break;

        default:
            LPC_GPIO1->FIOCLR = (1 << 19);
            us_current_state = US_IDLE;
            break;
    }
}
#include "EmetteurDTMF.h"

typedef enum {
    DTMF_IDLE,
    DTMF_START_CHAR,
    DTMF_WAIT_CHAR,
    DTMF_WAIT_PAUSE
} dtmf_seq_state_t;

static dtmf_seq_state_t seq_state = DTMF_IDLE;
static char cmd_buffer[8];
static uint8_t cmd_index = 0;
static uint32_t dtmf_timer = 0;

extern volatile uint32_t tick_ms;
#define SAMPLING_FREQ 40000
#define SINE_STEPS 1024

#define CALCULATE_INCREMENT(freq)  ((uint32_t)(((uint64_t)(freq) * 4194304ULL) / SAMPLING_FREQ))


static const uint32_t DTMF_STEP_LOW[4] = {
    CALCULATE_INCREMENT(697),
    CALCULATE_INCREMENT(770),
    CALCULATE_INCREMENT(852),
    CALCULATE_INCREMENT(941)
};

static const uint32_t DTMF_STEP_HIGH[4] = {
    CALCULATE_INCREMENT(1209),
    CALCULATE_INCREMENT(1336),
    CALCULATE_INCREMENT(1477),
    CALCULATE_INCREMENT(1633)
};

static uint16_t SINE_TABLE[SINE_STEPS];
volatile uint32_t current_step_low = 0;
volatile uint32_t current_step_high = 0;
volatile uint8_t sound_enabled = 0;

void init_timer(void) {
		for (int i = 0; i < SINE_STEPS; i++) {
        SINE_TABLE[i] = (uint16_t)(512.0 + 500.0 * sin((2.0 * 3.14159265 * i) / SINE_STEPS));
    }
    LPC_SC->PCONP |= (1 << 15);          
    LPC_PINCON->PINSEL1 &= ~(3 << 20);   
    LPC_PINCON->PINSEL1 |= (2 << 20);    
    
    LPC_SC->PCONP |= (1 << 1);         
    
    LPC_SC->PCLKSEL0 &= ~(3 << 2);    
    LPC_SC->PCLKSEL0 |= (1 << 2);
    
    LPC_TIM0->PR = 0;                 
    LPC_TIM0->MR0 = 2499;                
    
    LPC_TIM0->MCR |= (1 << 0) | (1 << 1); 
    
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = 1;   
    LPC_DAC->DACCNTVAL = 0;
}
/*
void TIMER0_IRQHandler(void) {
    if (LPC_TIM0->IR & (1 << 0)) {
        LPC_TIM0->IR = (1 << 0);
        
        static uint32_t phase_acc_low = 0;
       
        phase_acc_low += DTMF_STEP_LOW[0];
        
        uint32_t idx_low = (phase_acc_low >> 22) & 0x3FF;
        
        uint32_t dac_signal = SINE_TABLE[idx_low];
       
        LPC_DAC->DACR = (dac_signal << 6) | (1 << 16);
    }
}*/


void TIMER0_IRQHandler(void) {
    if (LPC_TIM0->IR & (1 << 0)) {
        LPC_TIM0->IR = (1 << 0);
        
      
        static uint32_t phase_acc_low = 0;
        static uint32_t phase_acc_high = 0;
        
        if (sound_enabled) {
            
            phase_acc_low  += current_step_low;
            phase_acc_high += current_step_high;
            
            uint32_t idx_low  = (phase_acc_low  >> 22) & 0x3FF;
            uint32_t idx_high = (phase_acc_high >> 22) & 0x3FF;
            
            uint32_t dac_signal = (SINE_TABLE[idx_low] + SINE_TABLE[idx_high]) >> 1;
						//uint32_t dac_signal = SINE_TABLE[idx_low];
           
            LPC_DAC->DACR = (dac_signal << 6) | (1 << 16);
        } else {
            LPC_DAC->DACR = (512 << 6) | (1 << 16);
				}
    }
}

void dtmf_set_char(char c) {
    int row = -1;
    int col = -1;

    if (c >= '1' && c <= '3') { row = 0; }
    else if (c >= '4' && c <= '6') { row = 1; }
    else if (c >= '7' && c <= '9') { row = 2; }
    else if (c == '*' || c == '0' || c == '#' || c == 'D' || c == 'd') { row = 3; }
    else if (c == 'A' || c == 'a') { row = 0; }
    else if (c == 'B' || c == 'b') { row = 1; }
    else if (c == 'C' || c == 'c') { row = 2; }

    if (c == '1' || c == '4' || c == '7' || c == '*') { col = 0; }
    else if (c == '2' || c == '5' || c == '8' || c == '0') { col = 1; }
    else if (c == '3' || c == '6' || c == '9' || c == '#') { col = 2; }
    else if (c == 'A' || c == 'a' || c == 'B' || c == 'b' || c == 'C' || c == 'c' || c == 'D' || c == 'd') { col = 3; }

    if (row != -1 && col != -1) {
        current_step_low = DTMF_STEP_LOW[row];
        current_step_high = DTMF_STEP_HIGH[col];
        sound_enabled = 1;
    } else {
        sound_enabled = 0;
    }
}

void dtmf_stop_sound(void) {
    sound_enabled = 0;
    current_step_low = 0;
    current_step_high = 0;
}

void dtmf_send_command(uint8_t robot_id, char action) {
    if (seq_state != DTMF_IDLE) return;

    cmd_buffer[0] = '#';
    if (robot_id < 10) {
        cmd_buffer[1] = robot_id + '0';
        cmd_buffer[2] = action;
        cmd_buffer[3] = '*';
        cmd_buffer[4] = '\0';
    } else {
        cmd_buffer[1] = (robot_id / 10) + '0';
        cmd_buffer[2] = (robot_id % 10) + '0';
        cmd_buffer[3] = action;
        cmd_buffer[4] = '*';
        cmd_buffer[5] = '\0';
    }

    cmd_index = 0;
    seq_state = DTMF_START_CHAR;
}

void dtmf_sequence_process_non_blocking(void) {
    switch (seq_state) {
        case DTMF_IDLE:
            break;

        case DTMF_START_CHAR:
            if (cmd_buffer[cmd_index] == '\0') {
                dtmf_stop_sound();
                seq_state = DTMF_IDLE;
            } else {
                dtmf_set_char(cmd_buffer[cmd_index]);
                dtmf_timer = tick_ms;
                seq_state = DTMF_WAIT_CHAR;
            }
            break;

        case DTMF_WAIT_CHAR:
            if ((uint32_t)(tick_ms - dtmf_timer) >= 100) {
                dtmf_stop_sound();
                dtmf_timer = tick_ms;
                seq_state = DTMF_WAIT_PAUSE;
            }
            break;

        case DTMF_WAIT_PAUSE:
            if ((uint32_t)(tick_ms - dtmf_timer) >= 50) {
                cmd_index++;
                seq_state = DTMF_START_CHAR;
            }
            break;

        default:
            seq_state = DTMF_IDLE;
            break;
    }
}
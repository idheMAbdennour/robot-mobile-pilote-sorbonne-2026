#include "RecepteurIR.h"

volatile LastSeenRobot last_robot = {0, '0', 'L', 0};

typedef enum {
    IR_STATE_WAIT_START_RISE, 
    IR_STATE_WAIT_START_FALL, 
    IR_STATE_VALIDATE_START,   
    IR_STATE_READ_BITS        
} ir_state_t;

static ir_state_t current_ir_state = IR_STATE_WAIT_START_RISE;
static uint32_t start_pulse_time = 0;
static uint32_t first_montant_time = 0; 
static uint16_t ir_shift_reg = 0;
static int8_t bit_counter = 0;

static uint32_t dynamic_tau = 0;
static uint32_t threshold_1_5_tau = 0;

void init_recepteur_ir(void) {
    LPC_SC->PCONP |= (1 << 2);           
    LPC_SC->PCLKSEL0 &= ~(3 << 4);       
    LPC_TIM1->PR = 25 - 1;               
    LPC_TIM1->TCR = 1;                   

    LPC_PINCON->PINSEL1 &= ~(3 << 10);   
    LPC_GPIO0->FIODIR &= ~(1 << 21);     
    
    LPC_GPIOINT->IO0IntEnR |= (1 << 21); 
    LPC_GPIOINT->IO0IntEnF |= (1 << 21); 
    
    NVIC_EnableIRQ(EINT3_IRQn);          
}

void process_ir_edge(void) {
    if ((LPC_GPIOINT->IO0IntStatR & (1 << 21)) || (LPC_GPIOINT->IO0IntStatF & (1 << 21))) {
        
        uint32_t current_time = LPC_TIM1->TC;
        LPC_GPIOINT->IO0IntClr |= (1 << 21); 

        uint8_t pin_state = (LPC_GPIO0->FIOPIN & (1 << 21)) ? 1 : 0;

        switch (current_ir_state) {
            
            case IR_STATE_WAIT_START_RISE:
                if (pin_state == 1) {
                    start_pulse_time = current_time; 
                    first_montant_time = current_time; 
                    current_ir_state = IR_STATE_WAIT_START_FALL; 
                }
                break;

            case IR_STATE_WAIT_START_FALL:
                if (pin_state == 0) {
                    uint32_t pulse_duration = current_time - start_pulse_time; 

                    if (pulse_duration >= 140 && pulse_duration <= 320) {
                        dynamic_tau = pulse_duration;
                        threshold_1_5_tau = dynamic_tau + (dynamic_tau / 2); 
                        current_ir_state = IR_STATE_VALIDATE_START; 
                    } else {
                        current_ir_state = IR_STATE_WAIT_START_RISE;
                    }
                }
                break;

            case IR_STATE_VALIDATE_START:
                if (pin_state == 1) {
                    uint32_t total_start_duration = current_time - first_montant_time;
                    uint32_t expected_2_tau = dynamic_tau * 2;
                    uint32_t tolerance = expected_2_tau / 6; 

                    if (total_start_duration >= (expected_2_tau - tolerance) && 
                        total_start_duration <= (expected_2_tau + tolerance)) {
												
												char cote_poste = (est_zone_nord == 1) ? 'G' : 'D';
                        trigger_envelope_hardware(num_post, cote_poste);
                        
                        ir_shift_reg = 0;
                        bit_counter = 0;
                        start_pulse_time = current_time; 
                        current_ir_state = IR_STATE_READ_BITS; 
                    } else {
                        current_ir_state = IR_STATE_WAIT_START_RISE;
                    }
                }
                break;

            case IR_STATE_READ_BITS:
                if (pin_state == 0) {
                    uint32_t bit_pulse_duration = current_time - start_pulse_time;

                    uint8_t bit = (bit_pulse_duration > threshold_1_5_tau) ? 1 : 0;

                    ir_shift_reg = (ir_shift_reg << 1) | bit;
                    bit_counter++;

                    if (bit_counter == 16) {
                        uint8_t rx_id       = (ir_shift_reg >> 12) & 0x0F; 
                        uint8_t rx_speed    = (ir_shift_reg >> 8)  & 0x0F; 
                        uint8_t rx_status   = (ir_shift_reg >> 4)  & 0x0F; 
                        uint8_t rx_checksum = ir_shift_reg         & 0x0F; 

                        uint8_t calculated_sum = rx_id + rx_speed + rx_status; 
                        uint8_t calculated_checksum = (uint8_t)((~calculated_sum + 1) & 0x0F); 

                        if (calculated_checksum == rx_checksum) { 
                            last_robot.id = rx_id;
                            last_robot.speed_hex = (rx_speed < 10) ? ('0' + rx_speed) : ('A' + (rx_speed - 10)); 
                            
                            if (rx_status & 0x01)      last_robot.status_char = 'L'; 
                            else if (rx_status & 0x02) last_robot.status_char = 'E'; 
                            else if (rx_status & 0x04) last_robot.status_char = 'C'; 
                            else if (rx_status & 0x08) last_robot.status_char = 'D'; 
                            
                            last_robot.pending = 1; 
                        }
                        
                        current_ir_state = IR_STATE_WAIT_START_RISE;
                    }
                }
                
                if (pin_state == 1) {
                    start_pulse_time = current_time;
                }
                break;
        }
    }
}
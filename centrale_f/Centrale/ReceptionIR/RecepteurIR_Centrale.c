#include "RecepteurIR_Centrale.h"

typedef enum {
    IR_STATE_WAIT_START_RISE, 
    IR_STATE_WAIT_START_FALL, 
    IR_STATE_VALIDATE_START,   
    IR_STATE_READ_BITS        
} ir_state_t;

typedef struct {
    ir_state_t state;
    uint32_t start_pulse_time;
    uint32_t first_montant_time;
    uint16_t shift_reg;
    int8_t bit_counter;
    uint32_t dynamic_tau;
    uint32_t threshold_1_5_tau;
} IR_Receiver_t;

static IR_Receiver_t ir_north;
static IR_Receiver_t ir_south;

void init_recepteur_ir_centrale(void) {
    LPC_SC->PCONP |= (1 << 2);           
    LPC_SC->PCLKSEL0 &= ~(3 << 4);       
    LPC_TIM1->PR = 25 - 1;               
    LPC_TIM1->TCR = 1;                   

    LPC_PINCON->PINSEL1 &= ~((3 << 10) | (3 << 12));   
    LPC_GPIO0->FIODIR   &= ~((1 << IR_PIN_NORTH) | (1 << IR_PIN_SOUTH));     
    
    LPC_GPIOINT->IO0IntEnR |= (1 << IR_PIN_NORTH) | (1 << IR_PIN_SOUTH); 
    LPC_GPIOINT->IO0IntEnF |= (1 << IR_PIN_NORTH) | (1 << IR_PIN_SOUTH); 
    
    ir_north.state = IR_STATE_WAIT_START_RISE;
    ir_south.state = IR_STATE_WAIT_START_RISE;

    NVIC_EnableIRQ(EINT3_IRQn);          
}

void EINT3_IRQHandler(void) {

    if ((LPC_GPIOINT->IO0IntStatR & (1 << IR_PIN_NORTH)) || (LPC_GPIOINT->IO0IntStatF & (1 << IR_PIN_NORTH))) {
        process_ir_edge(IR_PIN_NORTH, 1);
        LPC_GPIOINT->IO0IntClr |= (1 << IR_PIN_NORTH);
    }
    if ((LPC_GPIOINT->IO0IntStatR & (1 << IR_PIN_SOUTH)) || (LPC_GPIOINT->IO0IntStatF & (1 << IR_PIN_SOUTH))) {
        process_ir_edge(IR_PIN_SOUTH, 0);
        LPC_GPIOINT->IO0IntClr |= (1 << IR_PIN_SOUTH);
    }
}

void process_ir_edge(uint8_t pin_num, uint8_t is_north) {
    uint32_t current_time = LPC_TIM1->TC;
    uint8_t pin_state = (LPC_GPIO0->FIOPIN & (1 << pin_num)) ? 1 : 0;
    
    IR_Receiver_t *p_ir = (is_north) ? &ir_north : &ir_south;

    switch (p_ir->state) {
        
        case IR_STATE_WAIT_START_RISE:
            if (pin_state == 1) {
                p_ir->start_pulse_time = current_time; 
                p_ir->first_montant_time = current_time; 
                p_ir->state = IR_STATE_WAIT_START_FALL; 
            }
            break;

        case IR_STATE_WAIT_START_FALL:
            if (pin_state == 0) {
                uint32_t pulse_duration = current_time - p_ir->start_pulse_time; 

                if (pulse_duration >= 140 && pulse_duration <= 320) {
                    p_ir->dynamic_tau = pulse_duration;
                    p_ir->threshold_1_5_tau = p_ir->dynamic_tau + (p_ir->dynamic_tau / 2); 
                    p_ir->state = IR_STATE_VALIDATE_START; 
                } else {
                    p_ir->state = IR_STATE_WAIT_START_RISE;
                }
            }
            break;

        case IR_STATE_VALIDATE_START:
            if (pin_state == 1) {
                uint32_t total_start_duration = current_time - p_ir->first_montant_time;
                uint32_t expected_2_tau = p_ir->dynamic_tau * 2;
                uint32_t tolerance = expected_2_tau / 6; 

                if (total_start_duration >= (expected_2_tau - tolerance) && 
                    total_start_duration <= (expected_2_tau + tolerance)) {
                    
                    p_ir->shift_reg = 0;
                    p_ir->bit_counter = 0;
                    p_ir->start_pulse_time = current_time; 
                    p_ir->state = IR_STATE_READ_BITS; 
                } else {
                    p_ir->state = IR_STATE_WAIT_START_RISE;
                }
            }
            break;

        case IR_STATE_READ_BITS:
            if (pin_state == 0) {
                uint32_t bit_pulse_duration = current_time - p_ir->start_pulse_time;
                uint8_t bit = (bit_pulse_duration > p_ir->threshold_1_5_tau) ? 1 : 0;

                p_ir->shift_reg = (p_ir->shift_reg << 1) | bit;
                p_ir->bit_counter++;

                if (p_ir->bit_counter == 16) {
                    uint8_t rx_id       = (p_ir->shift_reg >> 12) & 0x0F; 
                    uint8_t rx_speed    = (p_ir->shift_reg >> 8)  & 0x0F; 
                    uint8_t rx_status   = (p_ir->shift_reg >> 4)  & 0x0F; 
                    uint8_t rx_checksum = p_ir->shift_reg         & 0x0F; 

                    uint8_t calculated_sum = (rx_id + rx_speed + rx_status) & 0x0F; 
                    uint8_t calculated_checksum = (uint8_t)((~calculated_sum + 1) & 0x0F);
                    
                    if (calculated_checksum == rx_checksum) { 
                        if (rx_id < MAX_ROBOTS) {
                            robots_db[rx_id].robot_id = rx_id;
                            robots_db[rx_id].vitesse_actuelle = rx_speed;
                            
                            robots_db[rx_id].at_intersection_entrance = (is_north) ? 1 : 2;
                            
                            if (rx_status & 0x01)      robots_db[rx_id].status = 'L'; 
                            else if (rx_status & 0x02) robots_db[rx_id].status = 'E'; 
                            else if (rx_status & 0x04) robots_db[rx_id].status = 'C'; 
                            else if (rx_status & 0x08) robots_db[rx_id].status = 'D';
                        }
                    }
                    p_ir->state = IR_STATE_WAIT_START_RISE;
                }
            }
            
            if (pin_state == 1) {
                p_ir->start_pulse_time = current_time;
            }
            break;
    }
}
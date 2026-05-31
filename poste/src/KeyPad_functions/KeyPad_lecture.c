#include "LPC17xx.h"
#include "KeyPad_lecture.h"

extern volatile uint32_t tick_ms;

Message msg_buffer[4];
int msg_count = 0;

char buffer[25];
int index = 0;

char clavier[4][4] = {
    {'D', '#', '0', '*'},
    {'C', '9', '8', '7'}, 
    {'B', '6', '5', '4'}, 
    {'A', '3', '2', '1'}
};

typedef enum {
    INPUT_IDLE,
    INPUT_DIGIT1,
    INPUT_DIGIT2,
    INPUT_LETTER,
    INPUT_VALIDATE
} InputState;

InputState current_input_state = INPUT_IDLE;

static uint32_t display_timer = 0;
static int display_mode = -2;
static int display_blink_cycles = 0;
static int display_pin_state = 0;


void init_leds(void) {
    LPC_GPIO3->FIODIR |= (1 << 25) | (1 << 26);
    LPC_GPIO0->FIODIR |= (1 << 22);
   
    LPC_GPIO3->FIOSET = (1 << 25) | (1 << 26);
    LPC_GPIO0->FIOSET = (1 << 22);
}


void affichage_etat_lecture(int code) {
   
    LPC_GPIO3->FIOSET = (1 << 25) | (1 << 26); 
    LPC_GPIO0->FIOSET = (1 << 22);             

    display_mode = code;
    display_timer = tick_ms;
    display_blink_cycles = 0;
    display_pin_state = 0;

    if (code == 0) {
        LPC_GPIO3->FIOCLR = (1 << 25); 
    } else if (code == -1) { 
        LPC_GPIO3->FIOCLR = (1 << 26); 
    } else if (code == 1) { 
        LPC_GPIO0->FIOCLR = (1 << 22);
    } else if (code == 2 || code == 3) {
        display_blink_cycles = 10; 
    }
}


void handle_display_leds_non_blocking(void) {
    if (display_mode == -2) return;
		//affichage_leds(2);
    if (display_mode == 0 || display_mode == -1 || display_mode == 1) {
        if (tick_ms - display_timer >= 300) { 
            LPC_GPIO3->FIOSET = (1 << 25) | (1 << 26); 
            LPC_GPIO0->FIOSET = (1 << 22);
            display_mode = -2; 
        }
        return;
    }

    if (display_mode == 2 || display_mode == 3) {
        if (tick_ms - display_timer >= 100) { 
            display_timer = tick_ms;
            uint32_t pin = (display_mode == 2) ? (1 << 25) : (1 << 26);

            if (display_blink_cycles > 0) {
                if (display_pin_state == 0) {
                    LPC_GPIO3->FIOCLR = pin; 
                    display_pin_state = 1;
                } else {
                    LPC_GPIO3->FIOSET = pin; 
                    display_pin_state = 0;
                }
                display_blink_cycles--;
            } else {
                LPC_GPIO3->FIOSET = pin; 
                display_mode = -1;
            }
        }
    }
}

void reset_current_input(void) {
    index = 0;
    current_input_state = INPUT_IDLE;
}

void process_key(uint8_t key) {
    if (current_input_state == INPUT_IDLE) {
        if (key == '#') {
            buffer[0] = '#';
            index = 1;
            current_input_state = INPUT_DIGIT1;
            affichage_etat_lecture(0);
            return;
        } else {
            affichage_etat_lecture(-1);
            return;
        }
    }
    
    switch (current_input_state) {
        case INPUT_DIGIT1:
            if (key >= '0' && key <= '9') { 
                buffer[index++] = key;     
                current_input_state = INPUT_DIGIT2;
                affichage_etat_lecture(0); 
            } else {
                reset_current_input();
                affichage_etat_lecture(-1); 
            }
            break;
            
        case INPUT_DIGIT2:
            if (key >= '0' && key <= '9') {
                buffer[index++] = key;     
                current_input_state = INPUT_LETTER;
                affichage_etat_lecture(0); 
            } else {
                reset_current_input();
                affichage_etat_lecture(-1); 
            }
            break;
            
        case INPUT_LETTER:
            if (key >= 'A' && key <= 'D') { 
                buffer[index++] = key;      
                current_input_state = INPUT_VALIDATE;
                affichage_etat_lecture(0);
            } else {
                reset_current_input();
                affichage_etat_lecture(-1); 
            }
            break;
            
        case INPUT_VALIDATE:
            if (key == '*') { 
                buffer[index++] = '*';
                buffer[index] = '\0';
       
                if (msg_count < 4) { 
                    uint32_t c;
                    for(c = 0; c < 6; c++) {
                        msg_buffer[msg_count].text[c] = buffer[c];
                    }
                    msg_count++;
                    affichage_etat_lecture(2); 
                } else {
                    affichage_etat_lecture(1); 
                }
                reset_current_input();
            } else {
                reset_current_input();
                affichage_etat_lecture(-1); 
            }
            break;

        default:
            reset_current_input();
            break;
    }
}

void init_gpio(void) {
    LPC_PINCON->PINSEL0 &= ~(0xFFFF << 8); 
    LPC_GPIO0->FIODIR &= ~(0xFF << 4);  
    LPC_GPIO0->FIOCLR = (0xF << 4);    
    LPC_PINCON->PINMODE0 &= ~(0xFFFF << 8); 
}

void init_timer(void) {
    LPC_SC->PCONP |= (1 << 1);
    LPC_SC->PCLKSEL0 &= ~(3 << 2);     
    LPC_TIM0->MR0 = 200000;             
    LPC_TIM0->MCR |= (1 << 0) | (1 << 1);
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = 1;                 
}


void TIMER0_IRQHandler(void) {
    if (LPC_TIM0->IR & (1 << 0)) {
        LPC_TIM0->IR = (1 << 0); 
        
        static uint8_t key_pressed = 0; 
        uint32_t i;
        uint8_t key_found = 0;
        uint8_t current_code = 0;

        for (i = 0; i < 4; i++) {
            LPC_GPIO0->FIODIR |= (1 << (4 + i)); 
           
            __NOP(); __NOP(); __NOP();
            
            uint32_t pins = LPC_GPIO0->FIOPIN;

            if (!(pins & (1 << 8)))       { current_code = clavier[0][i]; key_found = 1; }
            else if (!(pins & (1 << 9)))  { current_code = clavier[1][i]; key_found = 1; }
            else if (!(pins & (1 << 10))) { current_code = clavier[2][i]; key_found = 1; }
            else if (!(pins & (1 << 11))) { current_code = clavier[3][i]; key_found = 1; }
            
            LPC_GPIO0->FIODIR &= ~(1 << (4 + i)); 
            
            if (key_found) break; 
        }

        if (key_found) {
            if (key_pressed == 0) { 
                process_key(current_code); 
                key_pressed = 1;          
            }
        } else {
            key_pressed = 0; 
        }
    }
}
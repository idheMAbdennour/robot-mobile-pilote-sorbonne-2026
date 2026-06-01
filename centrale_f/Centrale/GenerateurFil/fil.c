#include "fil.h"

typedef enum {
    MOTIF_0, MOTIF_o, MOTIF_1, MOTIF_i, MOTIF_e, MOTIF_E
} Motif_Type;

const uint32_t MOTIF_DURATION_TICKS[6] = {
    75,   // MOTIF_0
    125,  // MOTIF_o
    175,  // MOTIF_1
    225,  // MOTIF_i
    275,  // MOTIF_e
    325   // MOTIF_E
};

#define PAUSE_DURATION_TICKS  50  

typedef enum {
    STATE_EMISSION,
    STATE_PAUSE
} Gen_State;

volatile Gen_State current_state = STATE_PAUSE;
volatile uint32_t tick_counter = 0;
volatile bool toggle_50khz = false;

#define BUFFER_SIZE 128
volatile uint8_t motif_buffer[BUFFER_SIZE];
volatile uint16_t buffer_head = 0;
volatile uint16_t buffer_tail = 0;
volatile bool msg_in_progress = false;
volatile uint8_t active_motif = MOTIF_E; 

void Set_Nord_HiZ(void) {
    LPC_GPIO0->FIOSET = PIN_N_PMOS; 
    LPC_GPIO0->FIOCLR = PIN_N_NMOS; 
}

void Set_Sud_HiZ(void) {
    LPC_GPIO0->FIOSET = PIN_S_PMOS; 
    LPC_GPIO0->FIOCLR = PIN_S_NMOS; 
}

void Toggle_Nord_50kHz(bool state) {
    if (state) { 
        LPC_GPIO0->FIOSET = PIN_N_PMOS;
        LPC_GPIO0->FIOCLR = PIN_N_NMOS;
        __NOP(); __NOP();
        LPC_GPIO0->FIOCLR = PIN_N_PMOS; 
    } else { 
        LPC_GPIO0->FIOSET = PIN_N_PMOS;
        LPC_GPIO0->FIOCLR = PIN_N_NMOS;
        __NOP(); __NOP();
        LPC_GPIO0->FIOSET = PIN_N_NMOS; 
    }
}

void Toggle_Sud_50kHz(bool state) {
    if (state) {
        LPC_GPIO0->FIOSET = PIN_S_PMOS;
        LPC_GPIO0->FIOCLR = PIN_S_NMOS;
        __NOP(); __NOP();
        LPC_GPIO0->FIOCLR = PIN_S_PMOS;
    } else {
        LPC_GPIO0->FIOSET = PIN_S_PMOS;
        LPC_GPIO0->FIOCLR = PIN_S_NMOS;
        __NOP(); __NOP();
        LPC_GPIO0->FIOSET = PIN_S_NMOS;
    }
}

void Generator_Init(void) {
    LPC_GPIO0->FIODIR |= (PIN_N_PMOS | PIN_N_NMOS | PIN_S_PMOS | PIN_S_NMOS | PIN_DEBUG);
    
    Set_Nord_HiZ();
    Set_Sud_HiZ();
    LPC_GPIO0->FIOCLR = PIN_DEBUG;

    LPC_SC->PCONP |= (1 << 1);          
    LPC_TIM0->MR0 = (SystemCoreClock / 4) / 100000 - 1; 
    LPC_TIM0->MCR = (1 << 0) | (1 << 1); 
    
    NVIC_SetPriority(TIMER0_IRQn, 1);    
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = 1;                  
}

void Push_Motif(uint8_t motif) {
    uint16_t next = (buffer_head + 1) % BUFFER_SIZE;
    if (next != buffer_tail) {
        motif_buffer[buffer_head] = motif;
        buffer_head = next;
    }
}

void TIMER0_IRQHandler(void) {
    LPC_TIM0->IR = 1; 
    
    if (tick_counter > 0) {
        tick_counter--;
        
        if (current_state == STATE_EMISSION) {
            toggle_50khz = !toggle_50khz;
            
            if (active_motif == MOTIF_0 || active_motif == MOTIF_1 || active_motif == MOTIF_E) {
                Toggle_Nord_50kHz(toggle_50khz);
                Set_Sud_HiZ(); 
            } else {
                Toggle_Sud_50kHz(toggle_50khz);
                Set_Nord_HiZ();
            }
        }
        return; 
    }

    if (current_state == STATE_EMISSION) {
        Set_Nord_HiZ();
        Set_Sud_HiZ();
        current_state = STATE_PAUSE;
        tick_counter = PAUSE_DURATION_TICKS; 
    } 
    else { 
        if (buffer_tail != buffer_head) {
            if (!msg_in_progress) {
                msg_in_progress = true;
                LPC_GPIO0->FIOSET = PIN_DEBUG; 
            }
            active_motif = motif_buffer[buffer_tail];
            buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
        } 
        else {
            if (msg_in_progress) {
                msg_in_progress = false;
                LPC_GPIO0->FIOCLR = PIN_DEBUG; 
            }
            
            static bool idle_flip = false;
            active_motif = idle_flip ? MOTIF_E : MOTIF_e;
            idle_flip = !idle_flip;
        }

        current_state = STATE_EMISSION;
        tick_counter = MOTIF_DURATION_TICKS[active_motif];
        toggle_50khz = true; 
    }
}

void Encode_And_Push_Bit(bool bit_val) {
    if (bit_val) {
        Push_Motif(MOTIF_1); 
        Push_Motif(MOTIF_i); 
    } else {
        Push_Motif(MOTIF_0); 
        Push_Motif(MOTIF_o); 
    }
}

void Generator_Send_Frame(uint8_t type, uint8_t robot_id, uint16_t parameter) {
    Push_Motif(MOTIF_E);
    Push_Motif(MOTIF_e);

    for (int i = 2; i >= 0; i--) {
        Encode_And_Push_Bit((type >> i) & 0x01);
    }

    for (int i = 3; i >= 0; i--) {
        Encode_And_Push_Bit((robot_id >> i) & 0x01);
    }

    for (int i = 6; i >= 0; i--) {
        Encode_And_Push_Bit((parameter >> i) & 0x01);
    }
}

void Generator_Cmd_Vitesse(uint8_t robot_id, uint8_t vitesse_pourcent) {
    Generator_Send_Frame(0x00, robot_id, vitesse_pourcent);
}

void Generator_Cmd_Station(uint8_t type_msg, uint8_t robot_id, uint8_t affichage_LL, uint8_t poste_yy) {
    uint16_t param = ((affichage_LL & 0x03) << 5) | (poste_yy & 0x1F);
    Generator_Send_Frame(type_msg, robot_id, param);
}
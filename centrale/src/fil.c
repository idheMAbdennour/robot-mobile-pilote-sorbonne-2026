#include "fil.h"

// Nouveau mapping sur le PORT 0 (évite soigneusement toutes les broches interdites)
#define PIN_N_HIN     (1 << 4)  // Branche Nord : High-Side Input -> Broche P0.4
#define PIN_N_LIN     (1 << 5)  // Branche Nord : Low-Side Input  -> Broche P0.5
#define PIN_S_HIN     (1 << 6)  // Branche Sud  : High-Side Input -> Broche P0.6
#define PIN_S_LIN     (1 << 7)  // Branche Sud  : Low-Side Input  -> Broche P0.7
#define PIN_DEBUG     (1 << 8)  // Patte de synchro              -> Broche P0.8

// Énumération des motifs disponibles
typedef enum {
    MOTIF_0, MOTIF_o, MOTIF_1, MOTIF_i, MOTIF_e, MOTIF_E
} Motif_Type;

// Tableau des durées d'émission en nombre de "ticks" de 10 µs
const uint32_t MOTIF_DURATION_TICKS[] = {
    [MOTIF_0] = 75,   // 0.75 ms
    [MOTIF_o] = 125,  // 1.25 ms
    [MOTIF_1] = 175,  // 1.75 ms
    [MOTIF_i] = 225,  // 2.25 ms
    [MOTIF_e] = 275,  // 2.75 ms
    [MOTIF_E] = 325   // 3.25 ms
};

#define PAUSE_DURATION_TICKS  50  // 0.5 ms

// États de la machine d'état (ISR)
typedef enum {
    STATE_EMISSION,
    STATE_PAUSE
} Gen_State;

// Variables globales de gestion du flux
volatile Gen_State current_state = STATE_PAUSE;
volatile uint32_t tick_counter = 0;
volatile bool toggle_50khz = false;


// Buffer circulaire pour stocker la séquence de motifs à envoyer
#define BUFFER_SIZE 128
volatile uint8_t motif_buffer[BUFFER_SIZE];
volatile uint16_t buffer_head = 0;
volatile uint16_t buffer_tail = 0;
volatile bool msg_in_progress = false;
volatile uint8_t active_motif = MOTIF_E; // Tracks the motif currently being emitted

// --- Fonctions de bas niveau pour le pilotage des ponts ---

void Set_Nord_HiZ(void) {
    LPC_GPIO0->FIOCLR = PIN_N_HIN; // Désactive High-Side (NMOS Haut OFF)
    LPC_GPIO0->FIOCLR = PIN_N_LIN; // Désactive Low-Side  (NMOS Bas OFF)
}

void Set_Sud_HiZ(void) {
    LPC_GPIO0->FIOCLR = PIN_S_HIN; // Désactive High-Side (NMOS Haut OFF)
    LPC_GPIO0->FIOCLR = PIN_S_LIN; // Désactive Low-Side  (NMOS Bas OFF)
}

void Toggle_Nord_50kHz(bool state) {
    if (state) { // État Haut du signal (VCC sur le fil)
        LPC_GPIO0->FIOCLR = PIN_N_LIN; // Éteint d'abord le bas (sécurité anti-cross conduction)
        LPC_GPIO0->FIOSET = PIN_N_HIN; // Allume le haut
    } else { // État Bas du signal (GND sur le fil)
        LPC_GPIO0->FIOCLR = PIN_N_HIN; // Éteint d'abord le haut
        LPC_GPIO0->FIOSET = PIN_N_LIN; // Allume le bas
    }
}

void Toggle_Sud_50kHz(bool state) {
    if (state) { // État Haut du signal (VCC sur le fil)
        LPC_GPIO0->FIOCLR = PIN_S_LIN; // Éteint d'abord le bas
        LPC_GPIO0->FIOSET = PIN_S_HIN; // Allume le haut
    } else { // État Bas du signal (GND sur le fil)
        LPC_GPIO0->FIOCLR = PIN_S_HIN; // Éteint d'abord le haut
        LPC_GPIO0->FIOSET = PIN_S_LIN; // Allume le bas
    }
}

// --- Initialisation du Matériel ---
void Generator_Init(void) {
    // Configuration des directions GPIO en sortie
    LPC_GPIO0->FIODIR |= (PIN_N_HIN | PIN_N_LIN | PIN_S_HIN | PIN_S_LIN | PIN_DEBUG);
    
    // Extinction initiale (Hi-Z partout : entrées IR2304 à 0)
    Set_Nord_HiZ();
    Set_Sud_HiZ();
    LPC_GPIO0->FIOCLR = PIN_DEBUG;

    // Configuration de TIMER0 pour une interruption toutes les 10 µs
    LPC_SC->PCONP |= (1 << 1);          // Alim Timer 0
    LPC_TIM0->MR0 = (SystemCoreClock / 4) / 100000 - 1; // 10 µs match
    LPC_TIM0->MCR = (1 << 0) | (1 << 1); // Interruption et Reset sur MR0
    
    // Priorité et activation
    NVIC_SetPriority(TIMER0_IRQn, 1);    
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR =  1;                  // Démarrage du Timer
}

// --- Gestion des buffers ---
void Push_Motif(uint8_t motif) {
    uint16_t next = (buffer_head + 1) % BUFFER_SIZE;
    if (next != buffer_tail) {
        motif_buffer[buffer_head] = motif;
        buffer_head = next;
    }
}

// --- L'interruption Critique : Base de temps 10 µs ---
void TIMER0_IRQHandler(void) {
    LPC_TIM0->IR = 1; // Clear interruption flag
    
    if (tick_counter > 0) {
        tick_counter--;
        
        // If we are in the emission phase, oscillate the appropriate wire at 50 kHz
        if (current_state == STATE_EMISSION) {
            toggle_50khz = !toggle_50khz;
            
            // Check which wire the active motif belongs to
            if (active_motif == MOTIF_0 || active_motif == MOTIF_1 || active_motif == MOTIF_E) {
                Toggle_Nord_50kHz(toggle_50khz);
                Set_Sud_HiZ(); // Safety: keep opposite line strictly isolated
            } else {
                Toggle_Sud_50kHz(toggle_50khz);
                Set_Nord_HiZ();
            }
        }
        return; // End of the 10 µs tick
    }

    // --- State Machine Transition (When tick_counter hits 0) ---
    if (current_state == STATE_EMISSION) {
        // End of burst -> Force total silence (Hi-Z) during the inter-motif pause
        Set_Nord_HiZ();
        Set_Sud_HiZ();
        current_state = STATE_PAUSE;
        tick_counter = PAUSE_DURATION_TICKS; // 50 ticks = 0.5 ms
    } 
    else { // End of pause -> Load the next motif to emit
        if (buffer_tail != buffer_head) {
            // A real message frame is waiting in the buffer!
            if (!msg_in_progress) {
                msg_in_progress = true;
                LPC_GPIO0->FIOSET = PIN_DEBUG; // Debug pin goes HIGH safely BEFORE the first burst starts
            }
            // Correct sequential read (Read first, then advance pointer)
            active_motif = motif_buffer[buffer_tail];
            buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
        } 
        else {
            // Buffer is completely empty -> Revert to background IDLE mode
            if (msg_in_progress) {
                msg_in_progress = false;
                LPC_GPIO0->FIOCLR = PIN_DEBUG; // Debug pin drops back to LOW immediately when the frame finishes
            }
            
            static bool idle_flip = false;
            active_motif = idle_flip ? MOTIF_E : MOTIF_e;
            idle_flip = !idle_flip;
        }

        current_state = STATE_EMISSION;
        tick_counter = MOTIF_DURATION_TICKS[active_motif];
        toggle_50khz = true; // Ready to toggle on the very next 10 µs tick
    }
}

// --- Fonctions Utilitaires d'encodage des commandes ---

// Ajoute un bit utile dans le buffer en respectant la stricte alternance Nord/Sud
void Encode_And_Push_Bit(bool bit_val) {
    if (bit_val) {
        Push_Motif(MOTIF_1); // Bit 1 Nord
        Push_Motif(MOTIF_i); // Bit 1 Sud
    } else {
        Push_Motif(MOTIF_0); // Bit 0 Nord
        Push_Motif(MOTIF_o); // Bit 0 Sud
    }
}

void Generator_Send_Frame(uint8_t type, uint8_t robot_id, uint16_t parameter) {
    // 1. Envoi des entêtes de début de trame
    Push_Motif(MOTIF_E);
    Push_Motif(MOTIF_e);

    // 2. Envoi du type de message (3 bits, MSB first)
    for (int i = 2; i >= 0; i--) {
        Encode_And_Push_Bit((type >> i) & 0x01);
    }

    // 3. Envoi du numéro de robot (4 bits, MSB first)
    for (int i = 3; i >= 0; i--) {
        Encode_And_Push_Bit((robot_id >> i) & 0x01);
    }

    // 4. Envoi du paramètre (7 bits, MSB first)
    for (int i = 6; i >= 0; i--) {
        Encode_And_Push_Bit((parameter >> i) & 0x01);
    }
}

// --- API Publique d'envoi des ordres spécifiés ---

void Generator_Cmd_Vitesse(uint8_t robot_id, uint8_t vitesse_pourcent) {
    Generator_Send_Frame(0x00, robot_id, vitesse_pourcent);
}

void Generator_Cmd_Station(uint8_t type_msg, uint8_t robot_id, uint8_t affichage_LL, uint8_t poste_yy) {
    // Construction du paramètre sur 7 bits : LL (2 bits) + yyyyy (5 bits)
    uint16_t param = ((affichage_LL & 0x03) << 5) | (poste_yy & 0x1F);
    Generator_Send_Frame(type_msg, robot_id, param);
}
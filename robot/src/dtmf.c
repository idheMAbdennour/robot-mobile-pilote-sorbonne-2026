#include "dtmf.h"

#define LED_CONCERNED_PIN   (1 << 8)
#define LED_STATUS_PIN      (1 << 9)

extern uint8_t ROBOT_NUMBER;

typedef enum {
    DTMF_STATE_IDLE,
    DTMF_STATE_READ_ROBOT_ID,
    DTMF_STATE_READ_ACTION,
    DTMF_STATE_WAIT_END
} dtmf_state_t;

typedef enum {
    ROBOT_STOPPED = 0,
    ROBOT_WAITING_JUNCTION,
    ROBOT_RUNNING
} robot_movement_status_t;

volatile uint8_t dtmf_tone = 0;
volatile char dtmf_char = ' ';
volatile uint8_t new_dtmf_flag = 0; 

volatile robot_movement_status_t robot_status = ROBOT_RUNNING; 
static uint32_t blink_counter = 0;

char DTMF_TABLE[16] = 
{
    'D', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', '0', '*',
    '#', 'A', 'B', 'C'
};

void init_dtmf()
{
    LPC_PINCON->PINMODE1 |= (0x3FF << 0);
    LPC_PINCON->PINSEL1 &= ~(0x3FF << 0);
    LPC_GPIO0->FIODIR &= ~(0x1F << 16);
    
    LPC_PINCON->PINSEL4 &= ~((3 << 16) | (3 << 18)); 
    
    LPC_GPIO2->FIODIR |= (LED_CONCERNED_PIN | LED_STATUS_PIN);
    
    LPC_GPIO2->FIOCLR = (LED_CONCERNED_PIN | LED_STATUS_PIN); 
    
    LPC_GPIOINT->IO0IntEnR |= (1 << 20);
    NVIC_EnableIRQ(EINT3_IRQn);
}

void process_dtmf_commands(void)
{
    static dtmf_state_t current_state = DTMF_STATE_IDLE;
    static uint8_t parsed_robot_id = 0;
    static char parsed_action = ' ';

    if (!new_dtmf_flag) {
        return;
    }
    
    new_dtmf_flag = 0;
    char c = dtmf_char;

    switch (current_state)
    {
        case DTMF_STATE_IDLE:
            if (c == '#') // En-tête de la séquence de l'aiguillage 
            {
                parsed_robot_id = 0;
                current_state = DTMF_STATE_READ_ROBOT_ID;
            }
            break;

        case DTMF_STATE_READ_ROBOT_ID:
            if (c >= '0' && c <= '9')
            {
                // Accumulation pour gérer les identifiants robots (ex: robot 11, robot 15...) 
                parsed_robot_id = (parsed_robot_id * 10) + (c - '0');
            }
            else if (c == 'A' || c == 'D') // Action détectée ('A' = Arrêt, 'D' = Départ) 
            {
                parsed_action = c;
                current_state = DTMF_STATE_WAIT_END;
            }
            else 
            {
                current_state = DTMF_STATE_IDLE; // Erreur de format
            }
            break;

        case DTMF_STATE_WAIT_END:
            if (c == '*') // Symbole de fin validé 
            {
                // Vérification si l'ordre s'adresse à notre robot (ou '0' pour un ordre général si prévu) 
                if (parsed_robot_id == ROBOT_NUMBER)
                {
                    // Allumer la LED qui valide que le message nous concerne (P2.8) 
                    LPC_GPIO2->FIOSET = LED_CONCERNED_PIN;
                    
                    if (parsed_action == 'A') // Ordre d'arrêt 
                    {
                        robot_status = ROBOT_WAITING_JUNCTION;
                        /*
													STOP MOTEUR
												*/
                    }
                    else if (parsed_action == 'D') // Ordre de départ 
                    {
                        robot_status = ROBOT_RUNNING;
                        /*
													DEMARRER MOTEUR
												*/
											
                    }
                }
                else
                {
                    // Ce message ne concerne pas notre robot, on éteint la LED concernée 
                    LPC_GPIO2->FIOCLR = LED_CONCERNED_PIN;
                }
                
                current_state = DTMF_STATE_IDLE;
            }
            else 
            {
                current_state = DTMF_STATE_IDLE;
            }
            break;

        default:
            current_state = DTMF_STATE_IDLE;
            break;
    }
}

void update_status_leds(void)
{
    // Contrôle de la LED d'état comportementale (P2.9)
    switch (robot_status)
    {
        case ROBOT_STOPPED:
            LPC_GPIO2->FIOCLR = LED_STATUS_PIN; // Éteinte 
            break;
            
        case ROBOT_WAITING_JUNCTION:
            // Clignotement 
            blink_counter++;
            if (blink_counter % 2 == 0) {
                LPC_GPIO2->FIOSET = LED_STATUS_PIN;
            } else {
                LPC_GPIO2->FIOCLR = LED_STATUS_PIN;
            }
            break;
            
        case ROBOT_RUNNING:
            LPC_GPIO2->FIOSET = LED_STATUS_PIN; // Allumée en continu 
            break;
    }
}



void EINT3_IRQHandler()
{
    if (LPC_GPIOINT->IO0IntStatR & (1 << 20))
    {
        // Lecture des données du décodeur (P0.16 à P0.19) 
        dtmf_tone = (LPC_GPIO0->FIOPIN >> 16) & 0xF;
        dtmf_char = DTMF_TABLE[dtmf_tone];
        new_dtmf_flag = 1; 
        
        LPC_GPIOINT->IO0IntClr = (1 << 20);
				process_dtmf_commands();
				update_status_leds();
    }
}
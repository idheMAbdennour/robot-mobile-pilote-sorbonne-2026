/**
 * @file dtmf.c
 * @brief Fichier du module dtmf.
 */

#include "dtmf.h"

#include "LPC17xx.h"

#include "led_register.h"
#include "robot_state.h"

/* ==========================================================================
 * DÉFINITIONS ET TYPES PRIVÉS
 * ========================================================================== */

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

/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */

static volatile uint8_t dtmf_tone = 0;
static volatile char dtmf_char = ' ';
static volatile uint8_t new_dtmf_flag = 0;

static volatile robot_movement_status_t robot_status = ROBOT_RUNNING;
static uint32_t blink_counter = 0;

static char dtmf_table[16] = {
    'D', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', '0', '*',
    '#', 'A', 'B', 'C'
};

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void process_dtmf_commands(void);
static void update_status_leds(void);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

void init_dtmf(void) {
    // Désactiver les pull-up/pull-down pour P0.16 à P0.19
    LPC_PINCON->PINMODE1 |= (0xFF << 0);
    LPC_PINCON->PINSEL1 &= ~(0xFF << 0);
    
    // Configurer P2.12 en GPIO (Valid)
    LPC_PINCON->PINSEL4 &= ~(3u << 24);
    
    // Configurer P0.16 à P0.19 en entrée
    LPC_GPIO0->FIODIR &= ~(PIN_DTMF_D0 | PIN_DTMF_D1 | PIN_DTMF_D2 | PIN_DTMF_D3);
    
    // Configurer P2.12 en entrée
    LPC_GPIO2->FIODIR &= ~PIN_DTMF_VALID;

    // Activer l'interruption sur front montant pour P2.12 (Valid)
    LPC_GPIOINT->IO2IntEnR |= PIN_DTMF_VALID;
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void process_dtmf_commands(void) {
    static dtmf_state_t current_state = DTMF_STATE_IDLE;
    static uint8_t parsed_robot_id = 0;
    static char parsed_action = ' ';

    if (!new_dtmf_flag) {
        return;
    }

    new_dtmf_flag = 0;
    char c = dtmf_char;

    switch (current_state) {
        case DTMF_STATE_IDLE:
            if (c == '#') { // En-tête de la séquence
                parsed_robot_id = 0;
                current_state = DTMF_STATE_READ_ROBOT_ID;
            }
            break;

        case DTMF_STATE_READ_ROBOT_ID:
            if (c >= '0' && c <= '9') {
                // Accumulation pour gérer les identifiants robots (ex: 11, 15...)
                parsed_robot_id = (parsed_robot_id * 10) + (c - '0');
            } else if (c == 'A' || c == 'D') { // Action détectée ('A' = Arrêt, 'D' = Départ)
                parsed_action = c;
                current_state = DTMF_STATE_WAIT_END;
            } else {
                current_state = DTMF_STATE_IDLE; // Erreur de format
            }
            break;

        case DTMF_STATE_WAIT_END:
            if (c == '*') { // Symbole de fin validé
                // Vérification si l'ordre s'adresse à notre robot
                if (parsed_robot_id == get_robot_number()) {
                    // Allumer la LED qui valide que le message nous concerne
                    led_register_set(LED_REG_DTMF_CONCERNE);

                    if (parsed_action == 'A') { // Ordre d'arrêt
                        robot_status = ROBOT_WAITING_JUNCTION;
                        // TODO: Arrêter les moteurs ici via pwm_set_moteurs(0, 0)
                    } else if (parsed_action == 'D') { // Ordre de départ
                        robot_status = ROBOT_RUNNING;
                        // TODO: Démarrer les moteurs ici
                    }
                } else {
                    // Ce message ne concerne pas notre robot, on éteint la LED
                    led_register_clr(LED_REG_DTMF_CONCERNE);
                }
                current_state = DTMF_STATE_IDLE;
            } else {
                current_state = DTMF_STATE_IDLE;
            }
            break;

        default:
            current_state = DTMF_STATE_IDLE;
            break;
    }
}

static void update_status_leds(void) {
    // Contrôle de la LED d'état comportementale
    switch (robot_status) {
        case ROBOT_STOPPED:
            led_register_clr(LED_REG_ETAT_JONCTION); // Éteinte
            break;

        case ROBOT_WAITING_JUNCTION:
            // Clignotement
            blink_counter++;
            if (blink_counter % 2 == 0) {
                led_register_set(LED_REG_ETAT_JONCTION);
            } else {
                led_register_clr(LED_REG_ETAT_JONCTION);
            }
            break;

        case ROBOT_RUNNING:
            led_register_set(LED_REG_ETAT_JONCTION); // Allumée en continu
            break;
    }
}

void dtmf_service(void) {
    process_dtmf_commands();
    update_status_leds();
}

void dtmf_interrupt_routine(void) {
    // Traitement DTMF (Interruption)
    if (LPC_GPIOINT->IO2IntStatR & PIN_DTMF_VALID) {
        // Lecture des données du décodeur (P0.16 à P0.19)
        dtmf_tone = (LPC_GPIO0->FIOPIN >> 16) & 0xF;
        dtmf_char = dtmf_table[dtmf_tone];
        new_dtmf_flag = 1;

        LPC_GPIOINT->IO2IntClr = PIN_DTMF_VALID; // Acquitter
    }
}

void test_dtmf_module(void) {
    // Test périodique dans le main
    dtmf_service();
}

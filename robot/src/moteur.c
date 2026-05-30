/**
 * @file moteur.c
 * @brief Fichier du module moteur.
 */

#include "moteur.h"

#include <stdio.h>

#include "LPC17xx.h"

#include "pwm.h"
#include "robot_state.h"
#include "uart.h"

/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */
static uint8_t mot_hw_mode = 0b11; // Mode matériel actuel (par défaut 11)
static uint8_t mot_wire_pending = 0;
static uint8_t mot_wire_mode = 0;

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void init_moteurs_switches(void);
static void moteurs_update_mode_from_gpio(void);
static void moteurs_send_frame(uint8_t mode, const char *prefix);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

void init_moteur_pwm(void) {
    pwm_init_moteurs();
}

void changer_pwm_gauche(uint8_t pourcent) {
    pwm_set_moteur_gauche(pourcent);
}

void changer_pwm_droite(uint8_t pourcent) {
    pwm_set_moteur_droit(pourcent);
}

void changer_pwm_moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite) {
    pwm_set_moteurs(pourcent_gauche, pourcent_droite);
}

void init_moteurs_debug(void) {
    init_moteurs_switches();
    moteurs_update_mode_from_gpio();
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void init_moteurs_switches(void) {
    // Configurer P0.4 et P0.5 en GPIO (PINSEL0 bits 8:9 et 10:11)
    LPC_PINCON->PINSEL0 &= ~((3u << 8) | (3u << 10));
    LPC_GPIO0->FIODIR &= ~(PIN_MOT_SW1 | PIN_MOT_SW2);

    LPC_GPIOINT->IO0IntEnR |= (PIN_MOT_SW1 | PIN_MOT_SW2);
    LPC_GPIOINT->IO0IntEnF |= (PIN_MOT_SW1 | PIN_MOT_SW2);
}

static void moteurs_update_mode_from_gpio(void) {
    uint8_t sw1 = (LPC_GPIO0->FIOPIN & PIN_MOT_SW1) ? 1 : 0;
    uint8_t sw2 = (LPC_GPIO0->FIOPIN & PIN_MOT_SW2) ? 1 : 0;
    mot_hw_mode = (sw2 << 1) | sw1;
}

void moteurs_interrupt_routine(void) {
    if (LPC_GPIOINT->IO0IntStatR & (PIN_MOT_SW1 | PIN_MOT_SW2) ||
        LPC_GPIOINT->IO0IntStatF & (PIN_MOT_SW1 | PIN_MOT_SW2)) {
        moteurs_update_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_MOT_SW1 | PIN_MOT_SW2);
    }
}

void moteurs_receive_wire_command(uint8_t wire_code) {
    if (wire_code == 0b100 || wire_code == 0b101) {
        mot_wire_mode = wire_code & 0x1u;
        mot_wire_pending = 1;
    }
}

static void moteurs_send_frame(uint8_t mode, const char *prefix) {
    char buffer[96];
    int offset = 0;
    int32_t pwm_g = 0;
    int32_t pwm_d = 0;
    int32_t v_moy = 0;
    int32_t w_ang = 0;

    if (prefix) {
        offset += sprintf(buffer + offset, "%s", prefix);
    }

    get_motor_pwms(&pwm_g, &pwm_d);
    get_motor_speeds(&v_moy, &w_ang);

    switch (mode) {
        case 0b00:
            offset += sprintf(buffer + offset, "W %lddeg/s\r\n", (long)w_ang);
            break;
        case 0b01:
            offset += sprintf(buffer + offset, "V %ldcm /s\r\n", (long)v_moy);
            break;
        case 0b10:
            offset += sprintf(buffer + offset, "G %ld D %ld\r\n", (long)pwm_g, (long)pwm_d);
            break;
        default:
            buffer[0] = '\0';
            break;
    }

    if (buffer[0] != '\0') {
        uart0_send_string(buffer);
    }
}

void debug_moteurs_send_frame(void) {
    if (mot_hw_mode == 0b11) {
        if (!mot_wire_pending) {
            return;
        }
        moteurs_send_frame(mot_wire_mode, "wire overwrite ");
        mot_wire_pending = 0;
        return;
    }
    moteurs_send_frame(mot_hw_mode, NULL);
}

void test_moteurs_module(void) {
    // Exemple d'utilisation du module moteur
    // On met les deux moteurs à 50% de puissance
    changer_pwm_moteurs(50, 50);
}

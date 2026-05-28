#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include <stdint.h>

// --- Configuration de Test ---
// Mettre à 1 pour forcer l'état des microswitchs à '11' (mode debug par fil)
#define FORCE_DEBUG_SWITCH_11 1
// Mettre à 1 pour forcer le mode de debug par fil à une valeur de test (1)
#define FORCE_WIRE_DEBUG_MODE_1 1

// Enumération pour l'état du robot à transmettre par IR
typedef enum {
    STATUS_LIBRE          = 0x01, // 0001
    STATUS_RDV_EXPEDITION = 0x02, // 0010
    STATUS_COLISPRIS      = 0x04, // 0100
    STATUS_RDV_DEPOSE     = 0x08  // 1000
} robot_status_t;

// --- Robot Global ---
void set_robot_number(uint8_t number);
uint8_t get_robot_number(void);

// --- Status IR ---
void set_robot_status(robot_status_t status);
robot_status_t get_robot_status(void);

// --- Vitesse et Commandes Moteurs ---
void set_robot_vitesse(uint8_t vitesse);
uint8_t get_robot_vitesse(void);
void set_motor_pwms(int32_t pwm_g, int32_t pwm_d);
void get_motor_pwms(int32_t *pwm_g, int32_t *pwm_d);
void set_motor_speeds(int32_t v_moy, int32_t w_ang);
void get_motor_speeds(int32_t *v_moy, int32_t *w_ang);

// --- Capteur Inductif (Distances et Angle) ---
void set_inductif_values(int32_t dist1, int32_t dist2, int32_t angle);
void get_inductif_values(int32_t *dist1, int32_t *dist2, int32_t *angle);

// --- Configurations Microswitchs ---
void set_microswitch_state(uint8_t state);
uint8_t get_microswitch_state(void);

// --- ITM Debug Globals ---
void set_debug_itm_enabled(uint8_t enabled);
uint8_t get_debug_itm_enabled(void);
void set_wire_debug_mode(uint8_t mode);
uint8_t get_wire_debug_mode(void);

#endif // ROBOT_STATE_H

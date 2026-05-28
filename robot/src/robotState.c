#include "robotState.h"

// Variables d'état globales privées (sécurisées par les getters/setters)
static uint8_t current_robot_number = 0; // Par exemple, le robot numéro 7 par défaut
static uint8_t current_vitesse = 0; // vitesse initiale
static robot_status_t current_status = STATUS_LIBRE; // Statut par défaut 0001

static int32_t current_pwm_g = 2;
static int32_t current_pwm_d = 3;
static int32_t current_v_moy = 4;
static int32_t current_w_ang = 5;

static int32_t current_dist1 = 1;
static int32_t current_dist2 = 2;
static int32_t current_angle = 3;

static uint8_t switch_state = 0;

static uint8_t itm_debug_enabled = 0;
static uint8_t wire_debug_mode = 0;

// ROBOT NUMBER
void set_robot_number(uint8_t number) {
    if (number <= 15) {
        current_robot_number = number;
    }
}

uint8_t get_robot_number(void) {
    return current_robot_number;
}

// VITESSE
void set_robot_vitesse(uint8_t vitesse) {
    if (vitesse <= 15) {
        current_vitesse = vitesse;
    }
}

uint8_t get_robot_vitesse(void) {
    return current_vitesse;
}

// STATUS
void set_robot_status(robot_status_t status) {
    current_status = status;
}

robot_status_t get_robot_status(void) {
    return current_status;
}

// MOTEURS COMMANDES & RETOURS
void set_motor_pwms(int32_t pwm_g, int32_t pwm_d) {
    current_pwm_g = pwm_g;
    current_pwm_d = pwm_d;
}

void get_motor_pwms(int32_t *pwm_g, int32_t *pwm_d) {
    if (pwm_g) *pwm_g = current_pwm_g;
    if (pwm_d) *pwm_d = current_pwm_d;
}

void set_motor_speeds(int32_t v_moy, int32_t w_ang) {
    current_v_moy = v_moy;
    current_w_ang = w_ang;
}

void get_motor_speeds(int32_t *v_moy, int32_t *w_ang) {
    if (v_moy) *v_moy = current_v_moy;
    if (w_ang) *w_ang = current_w_ang;
}

// CAPTEUR INDUCTIF
void set_inductif_values(int32_t dist1, int32_t dist2, int32_t angle) {
    current_dist1 = dist1;
    current_dist2 = dist2;
    current_angle = angle;
}

void get_inductif_values(int32_t *dist1, int32_t *dist2, int32_t *angle) {
    if (dist1) *dist1 = current_dist1;
    if (dist2) *dist2 = current_dist2;
    if (angle) *angle = current_angle;
}

// SWITCHS
void set_microswitch_state(uint8_t state) {
    switch_state = state & 0x03; // Mask pour 2 bits
}

uint8_t get_microswitch_state(void) {
#if FORCE_DEBUG_SWITCH_11
    return 3; // Force l'état '11'
#else
    return switch_state;
#endif
}

// ITM DEBUG
void set_debug_itm_enabled(uint8_t enabled) {
    itm_debug_enabled = enabled;
}

uint8_t get_debug_itm_enabled(void) {
    return itm_debug_enabled;
}

void set_wire_debug_mode(uint8_t mode) {
    wire_debug_mode = mode;
}

uint8_t get_wire_debug_mode(void) {
#if FORCE_WIRE_DEBUG_MODE_1
    return 1;
#else
    return wire_debug_mode;
#endif
}

/**
 * @file robot_state.c
 * @brief Fichier du module robot_state.
 */

#include "robot_state.h"
#include "proximetre.h"
#include "LPC17xx.h"
/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */

// État Global
static uint8_t current_robot_number = 0; // Par défaut : Robot 0
static uint8_t current_vitesse = 0;      // Vitesse initiale
static robot_status_t current_status = STATUS_LIBRE; 

// Moteurs
static int32_t current_pwm_g = 2;
static int32_t current_pwm_d = 3;
static int32_t current_v_moy = 4;
static int32_t current_w_ang = 5;

// Capteur Inductif (Distances et Angle)
static int32_t current_dist_av = 1;
static int32_t current_dist_ar = 2;
static int32_t current_dist_mil = 3;
static int32_t current_angle = 3;

// Capteur Inductif (Valeurs brutes ADC)
static uint16_t current_avg_av = 0;
static uint16_t current_avg_ar = 0;
static uint16_t current_avg_hor = 0;

// Proximètre (Tableau des 72 mesures de 5°)
static int32_t proxi_dists[NUM_PROXI_MEASUREMENTS];

// Wire Frame (Enveloppe)
static wire_trame_t current_wire_trame = {0, 0, 0};
static uint8_t wire_trame_is_valid = 0;
static uint8_t new_wire_trame = 0;

// Variables SPI
static uint8_t spi_vg = 0;
static uint8_t spi_vd = 0;
static uint8_t spi_pg = 0;
static uint8_t spi_pd = 0;

// Drapeaux (Flags)
static uint8_t flag_50hz = 0;

// Séquence IR
static uint8_t ir_sequence[MAX_SEQ_LENGTH];
static uint8_t ir_seq_index = 0;
static uint8_t ir_seq_length = 0;
static uint8_t ir_frame_counter = 0;

/* ==========================================================================
 * IMPLÉMENTATION DES ACCESSEURS D'ÉTAT DU ROBOT
 * ========================================================================== */

void init_robot_id_switches(void) {
    // Configurer P1.18 à P1.21 en GPIO (bits 4 à 11 dans PINSEL3)
    LPC_PINCON->PINSEL3 &= ~((3u << 4) | (3u << 6) | (3u << 8) | (3u << 10));
    // Configurer P1.18 à P1.21 en entrée
    LPC_GPIO1->FIODIR &= ~(PIN_ROBOT_ID_1 | PIN_ROBOT_ID_2 | PIN_ROBOT_ID_3 | PIN_ROBOT_ID_4);
}

void update_robot_id_from_hardware(void) {
    uint8_t id = 0;
    if (LPC_GPIO1->FIOPIN & PIN_ROBOT_ID_1) id |= 1;
    if (LPC_GPIO1->FIOPIN & PIN_ROBOT_ID_2) id |= 2;
    if (LPC_GPIO1->FIOPIN & PIN_ROBOT_ID_3) id |= 4;
    if (LPC_GPIO1->FIOPIN & PIN_ROBOT_ID_4) id |= 8;
    
    // Fallback de sécurité si aucun switch n'est levé (le robot ne peut pas avoir l'ID 0 en fonctionnement normal)
    if (id == 0) id = 1; 
    set_robot_number(id);
}

// --- ROBOT NUMBER ---
void set_robot_number(uint8_t number) {
    if (number <= 15) {
        current_robot_number = number;
    }
}

uint8_t get_robot_number(void) {
    return current_robot_number;
}

// --- VITESSE ---
void set_robot_vitesse(uint8_t vitesse) {
    if (vitesse <= 15) {
        current_vitesse = vitesse;
    }
}

uint8_t get_robot_vitesse(void) {
    return current_vitesse;
}

// --- STATUS ---
void set_robot_status(robot_status_t status) {
    current_status = status;
}

robot_status_t get_robot_status(void) {
    return current_status;
}

// --- MOTEURS ---
void set_motor_pwms(int32_t pwm_g, int32_t pwm_d) {
    current_pwm_g = pwm_g;
    current_pwm_d = pwm_d;
}

void get_motor_pwms(int32_t *pwm_g, int32_t *pwm_d) {
#if SIMULATE_SENSOR_VALUES
    if (pwm_g) *pwm_g = 350;
    if (pwm_d) *pwm_d = 450;
#else
    if (pwm_g) *pwm_g = current_pwm_g;
    if (pwm_d) *pwm_d = current_pwm_d;
#endif
}

void set_motor_speeds(int32_t v_moy, int32_t w_ang) {
    current_v_moy = v_moy;
    current_w_ang = w_ang;
}

void get_motor_speeds(int32_t *v_moy, int32_t *w_ang) {
#if SIMULATE_SENSOR_VALUES
    if (v_moy) *v_moy = 30;
    if (w_ang) *w_ang = -5;
#else
    if (v_moy) *v_moy = current_v_moy;
    if (w_ang) *w_ang = current_w_ang;
#endif
}

// --- CAPTEUR INDUCTIF ---
void set_inductif_values(int32_t dist_av, int32_t dist_ar, int32_t dist_mil, int32_t angle) {
    current_dist_av = dist_av;
    current_dist_ar = dist_ar;
    current_dist_mil = dist_mil;
    current_angle = angle;
}

void get_inductif_values(int32_t *dist_av, int32_t *dist_ar, int32_t *dist_mil, int32_t *angle) {
#if SIMULATE_SENSOR_VALUES
    if (dist_av) *dist_av = 123;
    if (dist_ar) *dist_ar = -12;
    if (dist_mil) *dist_mil = 456;
    if (angle) *angle = -10;
#else
    if (dist_av) *dist_av = current_dist_av;
    if (dist_ar) *dist_ar = current_dist_ar;
    if (dist_mil) *dist_mil = current_dist_mil;
    if (angle) *angle = current_angle;
#endif
}

void set_capteur_averages(uint16_t avg_av, uint16_t avg_ar, uint16_t avg_hor) {
    current_avg_av = avg_av;
    current_avg_ar = avg_ar;
    current_avg_hor = avg_hor;
}

void get_capteur_averages(uint16_t *avg_av, uint16_t *avg_ar, uint16_t *avg_hor) {
#if SIMULATE_SENSOR_VALUES
    if (avg_av) *avg_av = 4095;
    if (avg_ar) *avg_ar = 2048;
    if (avg_hor) *avg_hor = 1024;
#else
    if (avg_av) *avg_av = current_avg_av;
    if (avg_ar) *avg_ar = current_avg_ar;
    if (avg_hor) *avg_hor = current_avg_hor;
#endif
}

// --- PROXIMÈTRE ---
void set_proxi_distances(const int32_t *dists) {
    if (!dists) return;
    for (int i = 0; i < 72; i++) {
        proxi_dists[i] = dists[i];
    }
}

void get_proxi_distances(int32_t *dists) {
    if (!dists) return;
#if SIMULATE_SENSOR_VALUES
    for (int i = 0; i < NUM_PROXI_MEASUREMENTS; i++) {
        dists[i] = 200 + ((i % 18) * 7);
    }
#else
    for (int i = 0; i < NUM_PROXI_MEASUREMENTS; i++) {
        dists[i] = proxi_dists[i];
    }
#endif
}

void set_proxi_distance_at_angle(int angle_deg, int32_t distance) {
    int index = (angle_deg + PROXI_MAX_ANGLE_DEG) / PROXI_STEP_DEG;
    if (index >= 0 && index < NUM_PROXI_MEASUREMENTS) {
        proxi_dists[index] = distance;
    }
}

void get_proxi_distance_at_angle(int angle_deg, int32_t *distance) {
    int index = (angle_deg + PROXI_MAX_ANGLE_DEG) / PROXI_STEP_DEG;
    if (distance && index >= 0 && index < NUM_PROXI_MEASUREMENTS) {
        *distance = proxi_dists[index];
    }
}

// --- WIRE TRAME ---
void set_wire_trame(const wire_trame_t *trame, uint8_t is_valid) {
    if (!trame) return;
    current_wire_trame.type = trame->type;
    current_wire_trame.robot_id = trame->robot_id;
    current_wire_trame.parameter = trame->parameter;
    wire_trame_is_valid = is_valid;
}

void get_wire_trame(wire_trame_t *trame, uint8_t *is_valid) {
    if (!trame) return;
    trame->type = current_wire_trame.type;
    trame->robot_id = current_wire_trame.robot_id;
    trame->parameter = current_wire_trame.parameter;
    if (is_valid) {
        *is_valid = wire_trame_is_valid;
    }
}

uint8_t get_new_wire_trame(void) {
    return new_wire_trame;
}

void clear_new_wire_trame(void) {
    new_wire_trame = 0;
}

void set_new_wire_trame(uint8_t is_new) {
    new_wire_trame = is_new;
}

// --- VARIABLES SPI ---
void set_spi_variables(uint8_t vg, uint8_t vd, uint8_t pg, uint8_t pd) {
    spi_vg = vg;
    spi_vd = vd;
    spi_pg = pg;
    spi_pd = pd;
}

void get_spi_variables(uint8_t *vg, uint8_t *vd, uint8_t *pg, uint8_t *pd) {
    if (vg) *vg = spi_vg;
    if (vd) *vd = spi_vd;
    if (pg) *pg = spi_pg;
    if (pd) *pd = spi_pd;
}

// --- FLAGS ---
void set_flag_50hz(uint8_t flag) {
    flag_50hz = flag;
}

uint8_t get_flag_50hz(void) {
    return flag_50hz;
}

// --- SÉQUENCE IR ---
void set_ir_sequence_at(uint8_t index, uint8_t value) {
    if (index < MAX_SEQ_LENGTH) {
        ir_sequence[index] = value;
    }
}

uint8_t get_ir_sequence_at(uint8_t index) {
    if (index < MAX_SEQ_LENGTH) {
        return ir_sequence[index];
    }
    return 0;
}

void set_ir_seq_index(uint8_t index) {
    ir_seq_index = index;
}

uint8_t get_ir_seq_index(void) {
    return ir_seq_index;
}

void set_ir_seq_length(uint8_t length) {
    ir_seq_length = length;
}

uint8_t get_ir_seq_length(void) {
    return ir_seq_length;
}

void set_ir_frame_counter(uint8_t counter) {
    ir_frame_counter = counter;
}

uint8_t get_ir_frame_counter(void) {
    return ir_frame_counter;
}

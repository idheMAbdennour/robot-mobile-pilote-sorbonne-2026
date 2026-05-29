#include "robotState.h"

// Variables d'état globales privées (sécurisées par les getters/setters)
static uint8_t current_robot_number = 0; // Par exemple, le robot numéro 7 par défaut
static uint8_t current_vitesse = 0; // vitesse initiale
static robot_status_t current_status = STATUS_LIBRE; // Statut par défaut 0001

static int32_t current_pwm_g = 2;
static int32_t current_pwm_d = 3;
static int32_t current_v_moy = 4;
static int32_t current_w_ang = 5;

static int32_t current_dist_av = 1;
static int32_t current_dist_ar = 2;
static int32_t current_dist_mil = 3;
static int32_t current_angle = 3;

static int32_t current_pos_av = 111;
static int32_t current_pos_ar = 222;
static int32_t current_pos_mil = 333;

static int32_t proxi_dists[72]; // 72 mesures de 5° pour couvrir 360°

static uint8_t uart_debug_enabled = 0;

// Valeurs moyennes brutes lues par le capteur inductif (12-bit ADC)
static uint16_t current_avg_av = 0;
static uint16_t current_avg_ar = 0;
static uint16_t current_avg_hor = 0;

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

// CAPTEUR INDUCTIF
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

// Capteur inductif (averages)
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

// PROXIMETRE
void set_proxi_distances(const int32_t *dists) {
    if (!dists) {
        return;
    }

    for (int i = 0; i < 72; i++) {
        proxi_dists[i] = dists[i];
    }
}

void get_proxi_distances(int32_t *dists) {
    if (!dists) {
        return;
    }

#if SIMULATE_SENSOR_VALUES
    for (int i = 0; i < 72; i++) {
        dists[i] = 200 + ((i % 18) * 7);
    }
#else
    for (int i = 0; i < 72; i++) {
        dists[i] = proxi_dists[i];
    }
#endif
}

static int normalize_angle(int angle_deg) {
    int normalized = angle_deg % 360;

    if (normalized < 0) {
        normalized += 360;
    }

    return normalized;
}

void set_proxi_distance_at_angle(int angle_deg, int32_t distance) {
    int index = normalize_angle(angle_deg) / 5; // 5° par mesure
    proxi_dists[index] = distance;
}

void get_proxi_distance_at_angle(int angle_deg, int32_t *distance) {
    int index = normalize_angle(angle_deg) / 5; // 5° par mesure
    if (distance) {
        *distance = proxi_dists[index];
    }
}

// UART DEBUG GLOBAUX
void set_debug_uart_enabled(uint8_t enabled) {
    uart_debug_enabled = enabled;
}

uint8_t get_debug_uart_enabled(void) {
    return uart_debug_enabled;
}

// --- Wire Frame Reception ---
static wire_trame_t current_wire_trame = {0, 0, 0};
static uint8_t wire_trame_is_valid = 0;
static uint8_t new_wire_trame = 0;

void set_wire_trame(const wire_trame_t *trame, uint8_t is_valid)
{
    if (!trame) {
        return;
    }

    // 1. Copier d'abord les données
    current_wire_trame.type = trame->type;
    current_wire_trame.robot_id = trame->robot_id;
    current_wire_trame.parameter = trame->parameter;

    // 2. Puis mettre à jour le flag de validité
    wire_trame_is_valid = is_valid;

    // 3. Enfin setter le flag new (appelé séparément par l'appelant)
    // (pour éviter les incohérences entre les données et les flags)
}

void get_wire_trame(wire_trame_t *trame, uint8_t *is_valid)
{
    if (!trame) {
        return;
    }

    trame->type = current_wire_trame.type;
    trame->robot_id = current_wire_trame.robot_id;
    trame->parameter = current_wire_trame.parameter;

    if (is_valid) {
        *is_valid = wire_trame_is_valid;
    }
}

uint8_t get_new_wire_trame(void)
{
    return new_wire_trame;
}

void clear_new_wire_trame(void)
{
    new_wire_trame = 0;
}

void set_new_wire_trame(uint8_t is_new)
{
    new_wire_trame = is_new;
}

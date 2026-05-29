#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include <stdint.h>

// --- Configuration de Test ---
// Mettre à 1 pour simuler toutes les valeurs de capteurs (pas de capteur branché)
#define SIMULATE_SENSOR_VALUES 0

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
void set_inductif_values(int32_t dist_av, int32_t dist_ar, int32_t dist_mil, int32_t angle);
void get_inductif_values(int32_t *dist_av, int32_t *dist_ar, int32_t *dist_mil, int32_t *angle);

// --- Capteurs inductifs bruts (ADC averages) ---
void set_capteur_averages(uint16_t avg_av, uint16_t avg_ar, uint16_t avg_hor);
void get_capteur_averages(uint16_t *avg_av, uint16_t *avg_ar, uint16_t *avg_hor);

// --- Proximetre ---
void set_proxi_distances(const int32_t *dists);
void get_proxi_distances(int32_t *dists);
void set_proxi_distance_at_angle(int angle_deg, int32_t distance);
void get_proxi_distance_at_angle(int angle_deg, int32_t *distance);

// --- UART Debug Global ---
void set_debug_uart_enabled(uint8_t enabled);
uint8_t get_debug_uart_enabled(void);

// --- Wire Frame Reception ---
typedef struct {
    uint8_t type;       // Type de message (3 bits)
    uint8_t robot_id;   // Numéro de robot destinataire (4 bits)
    uint16_t parameter; // Paramètre du message (7 bits)
} wire_trame_t;

void set_wire_trame(const wire_trame_t *trame, uint8_t is_valid);
void get_wire_trame(wire_trame_t *trame, uint8_t *is_valid);
uint8_t get_new_wire_trame(void);
void clear_new_wire_trame(void);
void set_new_wire_trame(uint8_t is_new);

#endif // ROBOT_STATE_H

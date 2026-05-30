/**
 * @file robot_state.h
 * @brief Fichier du module robot_state.
 */

#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS ID ROBOT)
 * ========================================================================== */
#define PIN_ROBOT_ID_1 (1u << 18) // P1.18 - Bit 0 ID
#define PIN_ROBOT_ID_2 (1u << 19) // P1.19 - Bit 1 ID
#define PIN_ROBOT_ID_3 (1u << 20) // P1.20 - Bit 2 ID
#define PIN_ROBOT_ID_4 (1u << 21) // P1.21 - Bit 3 ID

/* ==========================================================================
 * ACCESSEURS D'ÉTAT DU ROBOT
 * ========================================================================== */

/**
 * @brief Initialise les broches pour lire l'ID matériel du robot.
 */
void init_robot_id_switches(void);

/**
 * @brief Lit l'état des DIP switches matériels et met à jour l'ID du robot.
 */
void update_robot_id_from_hardware(void);

/**
 * @brief Définit le numéro du robot (ID).
 * @param number Le numéro du robot (ex: 1 à 15).
 */

/* ==========================================================================
 * DÉFINITIONS ET CONSTANTES
 * ========================================================================== */

// Configuration de Test : Mettre à 1 pour simuler les capteurs
#define SIMULATE_SENSOR_VALUES 0

// Longueur maximale d'une séquence IR
#define MAX_SEQ_LENGTH 100

// État du robot à transmettre par IR
typedef enum {
    STATUS_LIBRE          = 0b0001, // 1
    STATUS_RDV_EXPEDITION = 0b0010, // 2
    STATUS_COLISPRIS      = 0b0100, // 4
    STATUS_RDV_DEPOSE     = 0b1000  // 8
} robot_status_t;

// Structure de réception Wire (Enveloppe)
typedef struct {
    uint8_t type;       // Type de message (3 bits)
    uint8_t robot_id;   // Numéro de robot destinataire (4 bits)
    uint16_t parameter; // Paramètre du message (7 bits)
} wire_trame_t;

/* ==========================================================================
 * PROTOTYPES DES ACCESSEURS (GETTERS/SETTERS)
 * ========================================================================== */

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

// --- Capteur Inductif ---
void set_inductif_values(int32_t dist_av, int32_t dist_ar, int32_t dist_mil, int32_t angle);
void get_inductif_values(int32_t *dist_av, int32_t *dist_ar, int32_t *dist_mil, int32_t *angle);

void set_capteur_averages(uint16_t avg_av, uint16_t avg_ar, uint16_t avg_hor);
void get_capteur_averages(uint16_t *avg_av, uint16_t *avg_ar, uint16_t *avg_hor);

// --- Proximètre ---
void set_proxi_distances(const int32_t *dists);
void get_proxi_distances(int32_t *dists);

void set_proxi_distance_at_angle(int angle_deg, int32_t distance);
void get_proxi_distance_at_angle(int angle_deg, int32_t *distance);

// --- Wire Frame Reception ---
void set_wire_trame(const wire_trame_t *trame, uint8_t is_valid);
void get_wire_trame(wire_trame_t *trame, uint8_t *is_valid);

uint8_t get_new_wire_trame(void);
void clear_new_wire_trame(void);
void set_new_wire_trame(uint8_t is_new);

// --- Variables SPI (Vg, Vd, Pg, Pd) ---
void set_spi_variables(uint8_t vg, uint8_t vd, uint8_t pg, uint8_t pd);
void get_spi_variables(uint8_t *vg, uint8_t *vd, uint8_t *pg, uint8_t *pd);

// --- Drapeaux (Flags) Globaux ---
void set_flag_50hz(uint8_t flag);
uint8_t get_flag_50hz(void);

// --- Séquence IR ---
void set_ir_sequence_at(uint8_t index, uint8_t value);
uint8_t get_ir_sequence_at(uint8_t index);

void set_ir_seq_index(uint8_t index);
uint8_t get_ir_seq_index(void);

void set_ir_seq_length(uint8_t length);
uint8_t get_ir_seq_length(void);

void set_ir_frame_counter(uint8_t counter);
uint8_t get_ir_frame_counter(void);

#endif // ROBOT_STATE_H

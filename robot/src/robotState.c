#include "robotState.h"

// Variables d'état globales privées (sécurisées par les getters/setters)
static uint8_t current_robot_number = 0; // Par exemple, le robot numéro 7 par défaut
static uint8_t current_vitesse = 0; // vitesse initiale
static robot_status_t current_status = STATUS_LIBRE; // Statut par défaut 0001

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

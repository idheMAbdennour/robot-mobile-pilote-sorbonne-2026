#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include <stdint.h>

// Enumération pour l'état du robot à transmettre par IR
typedef enum {
    STATUS_LIBRE          = 0x01, // 0001
    STATUS_RDV_EXPEDITION = 0x02, // 0010
    STATUS_COLISPRIS      = 0x04, // 0100
    STATUS_RDV_DEPOSE     = 0x08  // 1000
} robot_status_t;

// --- Robot Number ---
void set_robot_number(uint8_t number);
uint8_t get_robot_number(void);

// --- Vitesse ---
void set_robot_vitesse(uint8_t vitesse);
uint8_t get_robot_vitesse(void);

// --- Status IR ---
void set_robot_status(robot_status_t status);
robot_status_t get_robot_status(void);

#endif // ROBOT_STATE_H

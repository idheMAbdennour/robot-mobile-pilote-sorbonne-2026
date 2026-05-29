#include "GestionAiguillage.h"

#define INTERSECTION_TIMEOUT_MS  2000

volatile uint8_t intersection_busy = 0;
volatile int8_t robot_inside_intersection = -1;
volatile int8_t robot_waiting_on_entrance = -1;
static uint32_t intersection_timer = 0;

extern void dtmf_send_command(uint8_t robot_id, char action);
extern volatile uint32_t tick_ms;

void gestion_intersection_process(void) {
    if (intersection_busy == 1 && robot_inside_intersection != -1) {
        if ((uint32_t)(tick_ms - intersection_timer) >= INTERSECTION_TIMEOUT_MS) {
            intersection_busy = 0;
            robot_inside_intersection = -1;
        }
    }

    if (robot_waiting_on_entrance != -1) {
        if (intersection_busy == 0) {
            uint8_t waiting_id = robot_waiting_on_entrance;
            robot_waiting_on_entrance = -1;
            
            intersection_busy = 1;
            robot_inside_intersection = waiting_id;
            intersection_timer = tick_ms;
            
            dtmf_send_command(waiting_id, 'D');
        }
        return;
    }

    for (uint8_t id = 0; id < MAX_ROBOTS; id++) {
        if (robots_db[id].robot_id == id && robots_db[id].at_intersection_entrance == 1) {
            robots_db[id].at_intersection_entrance = 0;

            if (intersection_busy == 0) {
                intersection_busy = 1;
                robot_inside_intersection = id;
                intersection_timer = tick_ms;
                dtmf_send_command(id, 'D');
            } 
            else {
                if (robot_inside_intersection != id) {
                    robot_waiting_on_entrance = id;
                    dtmf_send_command(id, 'A');
                }
            }
        }
    }
}
#include "debug.h"
#include "uart.h"
#include "robotState.h"
#include "capteurInductif.h"
#include <stdint.h>
#include <stdio.h>

static void send_motor_debug(void)
{
    char buffer[64];
    int32_t pwm_g;
    int32_t pwm_d;

    get_motor_pwms(&pwm_g, &pwm_d);
    sprintf(buffer, "G %ld D %ld\r\n", (long)pwm_g, (long)pwm_d);
    uart0_send_string(buffer);
}

static void send_motor_speed_debug(void)
{
    char buffer[64];
    int32_t v_moy;
    int32_t w_ang;

    get_motor_speeds(&v_moy, &w_ang);
    sprintf(buffer, "V %ldcm /s\r\n", (long)v_moy);
    uart0_send_string(buffer);
}

static void send_inductive_debug(void)
{
    char buffer[96];
    int32_t dist1;
    int32_t dist2;
    int32_t angle;
    uint32_t avg1;
    uint32_t avg2;
    uint32_t avg3;
    float period_ms;

    get_inductif_values(&dist1, &dist2, &angle);
    get_capteur_averages(&avg1, &avg2, &avg3);
    period_ms = get_envelope_period_ms();

    sprintf(buffer, "A %lu B %lu C %lu I %ld d1 %ld d2 %ld a %ld\r\n",
            (unsigned long)avg1,
            (unsigned long)avg2,
            (unsigned long)avg3,
            (long)period_ms,
            (long)dist1,
            (long)dist2,
            (long)angle);
    uart0_send_string(buffer);
}

static void send_wire_debug(void)
{
    uint8_t wire_mode = get_wire_debug_mode();

    switch (wire_mode)
    {
        case 1:
            send_inductive_debug();
            break;
        case 2:
            send_motor_debug();
            break;
        case 3:
            send_motor_speed_debug();
            break;
        default:
            uart0_send_string("DBG NONE\r\n");
            break;
    }
}

void debug_uart_send_frame(void)
{
    switch (get_microswitch_state())
    {
        case 3:
            send_wire_debug();
            break;
        case 2:
            send_motor_debug();
            break;
        case 1:
            send_motor_speed_debug();
            break;
        default:
            send_inductive_debug();
            break;
    }
}
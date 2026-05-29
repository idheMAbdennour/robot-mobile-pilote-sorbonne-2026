#include "LPC17xx.h"
#include "uart.h"
#include "robotState.h"
#include "moteur.h"
#include "capteurInductif.h"
#include "proximetre.h"
#include "emissionIR.h"
#include "decode_enveloppe.h"
#include <stdio.h>
#include <stdint.h>

volatile uint8_t flag_50hz = 0;

int main(void)
{
    SystemInit();

    init_uart0();
    init_PWM_IR();
    init_Timer_Enveloppe(250);
    init_moteurs_debug();
    init_proximetre();
    init_capteur_inductif();

    SysTick_Config(SystemCoreClock / 50);
    set_debug_uart_enabled(1);

    while (1)
    {
        if (!flag_50hz)
        {
            continue;
        }

        flag_50hz = 0;

        // Balayage non bloquant du proximètre : doit tourner même si le debug UART
        // est désactivé. La trame "T/t ddd..." est émise à la fin de chaque balayage
        // depuis proximetre_tick() (gardée par get_debug_uart_enabled()).
        proximetre_tick();

        if (!get_debug_uart_enabled()) {
            continue;
        }

        // Envois debug par module
        debug_moteurs_send_frame();
        debug_inductif_send_frame();
    }
}

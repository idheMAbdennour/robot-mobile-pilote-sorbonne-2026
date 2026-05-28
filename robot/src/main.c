#include "LPC17xx.h"
#include "uart.h"
#include "robotState.h"
#include "microswitchs.h"
#include "emissionIR.h"
#include "debug.h"
#include <stdint.h>

volatile uint8_t flag_50hz = 0;

int main(void)
{
    SystemInit();

    init_uart0();
    init_PWM_IR();
    init_Timer_Enveloppe(250);
    init_microswitchs();
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

        if (!get_debug_uart_enabled())
        {
            continue;
        }

        debug_uart_send_frame();
    }
}

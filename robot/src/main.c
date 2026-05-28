// Minimal debug main: delegate init to copy_main_init(), then send a test string periodically
#include "uart.h"
#include <stdint.h>
#include <stdio.h>

volatile uint8_t flag_50hz = 0; // réutilisé par SysTick

int main(void)
{
    // Perform a compact copy of the main initialisations into uart.c
    copy_main_init();

    // Small periodic debug message loop: every 200ms (flag set at 100Hz)
    const char *msg = "DEBUG UART0 alive\r\n";
    uint8_t counter = 0;

    while (1) {
        if (flag_50hz) {
            flag_50hz = 0;
            if (++counter >= 20) { // 20 * 10ms = 200ms
                uart0_send_string(msg);
                counter = 0;
            }
        }
    }
}

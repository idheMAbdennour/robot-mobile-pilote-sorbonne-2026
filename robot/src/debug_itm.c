#include "LPC17xx.h"
#include "debug_itm.h"
#include <stdio.h>

void init_debug_itm(void)
{
    // L'ITM/SWO est géré matériellement par le débogueur CMSIS-DAP/J-Link
    // et Keil µVision. Il n'y a pas besoin de configurer de périphériques
    // ou d'horloges comme pour l'UART dans le code.
    // L'activation se fait directement dans les options du projet Keil (onglet Debug -> Trace).
}

void itm_send_string(const char *str)
{
    while (*str)
    {
        // ITM_SendChar est une macro fournie nativement par core_cm3.h d'ARM
        ITM_SendChar(*str++);
    }
}

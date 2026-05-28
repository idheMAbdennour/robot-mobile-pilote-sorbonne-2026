#include "LPC17xx.h"
#include "proximetre.h"
#include "robotState.h"
#include "uart.h"
#include <stdio.h>

#define PIN_PROX_SW1 (1 << 12) // P0.12
#define PIN_PROX_SW2 (1 << 13) // P0.13

static uint8_t prox_mode = 0;

static void proximetre_update_mode_from_gpio(void)
{
    uint8_t sw1 = LPC_GPIO0->FIOPIN & PIN_PROX_SW1;
    uint8_t sw2 = LPC_GPIO0->FIOPIN & PIN_PROX_SW2;

    prox_mode = (sw2 << 1) | sw1;
}

void init_proximetre(void)
{
    init_proximetre_switches();
    proximetre_update_mode_from_gpio();
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void init_proximetre_switches(void)
{
    LPC_PINCON->PINSEL0 &= ~((3u << 24) | (3u << 26));
    LPC_GPIO0->FIODIR &= ~(PIN_PROX_SW1 | PIN_PROX_SW2);

    LPC_GPIOINT->IO0IntEnR |= (PIN_PROX_SW1 | PIN_PROX_SW2);
    LPC_GPIOINT->IO0IntEnF |= (PIN_PROX_SW1 | PIN_PROX_SW2);
}

void proximetre_interrupt_routine(void)
{
    if (LPC_GPIOINT->IO0IntStatR & (PIN_PROX_SW1 | PIN_PROX_SW2) ||
        LPC_GPIOINT->IO0IntStatF & (PIN_PROX_SW1 | PIN_PROX_SW2))
    {
        proximetre_update_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_PROX_SW1 | PIN_PROX_SW2);
    }
}

void debug_proximetre_send_frame(void)
{
    char buffer[128];
    int offset = 0;

    int32_t mesures[NUM_PROXI_MEASUREMENTS];
    char header = (prox_mode & 0x1u) ? 'T' : 't';
    get_proxi_distances(mesures);

    // En-tête
    offset += sprintf(buffer + offset, "%c ", header);

    // Ajout des mesures sur 3 chiffres
    for (int i = 0; i < NUM_PROXI_MEASUREMENTS; i++) {
        offset += sprintf(buffer + offset, "%03d ", mesures[i]);
    }

    // Fin de ligne imposée
    sprintf(buffer + offset, "\r\n"); // 0x0D 0x0A

    uart0_send_string(buffer);
}

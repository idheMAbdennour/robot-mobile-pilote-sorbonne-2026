#include "LPC17xx.h"
#include "uart.h"

void init_uart0(void)
{
    uint32_t pclk;
    uint16_t dl;

    // 1. Power UART0
    LPC_SC->PCONP |= (1 << 3);

    // 2. PCLK UART0 = CCLK/4
    LPC_SC->PCLKSEL0 &= ~(3 << 6);

    // 3. Configure P0.2 = TXD0, P0.3 = RXD0
    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |=  ((1 << 4) | (1 << 6));

    // 4. UART config : 8 bits, 1 stop, no parity
    // DLAB = 1
    LPC_UART0->LCR = 0x83;

    // 5. Baudrate = 115200
    // PCLK = SystemCoreClock / 4
    pclk = SystemCoreClock / 4;

    // Diviseur UART
    dl = pclk / (16 * 115200);

    LPC_UART0->DLM = (dl >> 8) & 0xFF;
    LPC_UART0->DLL = dl & 0xFF;

    // 6. DLAB = 0
    LPC_UART0->LCR = 0x03;

    // 7. Enable + reset FIFOs
    LPC_UART0->FCR = 0x07;
}

void uart0_send_char(char c)
{
    // Attendre THR vide
    while (!(LPC_UART0->LSR & (1 << 5)));

    LPC_UART0->THR = c;
}

void uart0_send_string(const char *str)
{
    while (*str)
    {
        uart0_send_char(*str++);
    }

    // Attendre fin transmission complète
    while (!(LPC_UART0->LSR & (1 << 6)));
}

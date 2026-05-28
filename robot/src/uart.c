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

    // 5. Baudrate = 11520
    // PCLK = SystemCoreClock / 4
    pclk = SystemCoreClock / 4;

    // Diviseur UART
    dl = pclk / (16 * 11520);

    LPC_UART0->DLM = (dl >> 8) & 0xFF;
    LPC_UART0->DLL = dl & 0xFF;

    // 6. DLAB = 0
    LPC_UART0->LCR = 0x03;

    // 7. Enable + reset FIFOs
    LPC_UART0->FCR = 0x07;
}

// Debug helpers: expose last written char and LSR value for inspection
volatile uint32_t debug_last_lsr = 0;
volatile uint32_t debug_last_thr = 0;

void uart0_send_char(char c)
{
    // Ensure P0.0 is GPIO output for visible toggle
    LPC_PINCON->PINSEL0 &= ~(3 << 0); // P0.0 = GPIO
    LPC_GPIO0->FIODIR |= (1 << 0);

    // Attendre THR vide (THRE)
    while (!(LPC_UART0->LSR & (1 << 5)));

    // Toggle debug pin high to mark start of THR write
    LPC_GPIO0->FIOSET = (1 << 0);

    // Record and write
    debug_last_thr = (uint32_t)c;
    LPC_UART0->THR = c;

    // small delay
    for (volatile int i = 0; i < 100; ++i) { __NOP(); }

    // Read back LSR for diagnostics
    debug_last_lsr = LPC_UART0->LSR;

    // Clear debug pin
    LPC_GPIO0->FIOCLR = (1 << 0);
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

// Copie des initialisations principales pour tests/debug
void copy_main_init(void)
{
    // Init system clocks before UART setup.
    SystemInit();

    // Initialisations Infrarouge (si présentes dans le projet)
    extern void init_PWM_IR(void);
    extern void init_Timer_Enveloppe(uint32_t);
    extern void init_microswitchs(void);
    extern void init_capteur_inductif(void);
    extern void set_debug_uart_enabled(uint8_t);

    // Initialisations UART et capteurs
    init_uart0();

    // Initialisations Infrarouge (si présentes dans le projet)
    init_PWM_IR();
    init_Timer_Enveloppe(250);

    init_microswitchs();
    init_capteur_inductif();

    // SysTick rapide pour tests (100Hz)
    SysTick_Config(SystemCoreClock / 100);

    // Activer le debug UART localement
    set_debug_uart_enabled(1);
}

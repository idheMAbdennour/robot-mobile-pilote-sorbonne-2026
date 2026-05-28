#include "LPC17xx.h"
#include "uart.h"

void init_uart0(void)
{
    // Init system clocks before UART setup.
    SystemInit();

    // 1. Allumer l'alimentation du bloc UART0 (Bit 3 du registre PCONP)
    LPC_SC->PCONP |= (1 << 3);

    // 2. Assurer que PCLK pour UART0 est réglé par défaut (PCLKSEL0 bits 6 et 7)
    LPC_SC->PCLKSEL0 &= ~(3 << 6);

    // 3. Configurer les broches P0.2 en TXD0 et P0.3 en RXD0
    // P0.2 -> PINSEL0 bits [5:4] = 01 (binaire)
    // P0.3 -> PINSEL0 bits [7:6] = 01 (binaire)
    LPC_PINCON->PINSEL0 &= ~(0xF << 4);
    LPC_PINCON->PINSEL0 |=  (0x5 << 4);

    // 4. Configuration UART0 (8 bits de données, 1 bit d'arrêt, pas de parité)
    LPC_UART0->LCR = 0x83; // DLAB = 1 permet l'accès aux diviseurs de baudrate

    // 5. Calcul Baudrate = 115200
    {
        uint32_t pclk = SystemCoreClock / 4;
        uint32_t div = pclk / (16 * 115200);
        LPC_UART0->DLL = div & 0xFF;
        LPC_UART0->DLM = (div >> 8) & 0xFF;
    }

    // 6. Désactiver DLAB (Verrouille le baudrate)
    LPC_UART0->LCR = 0x03;

    // 7. Activer les FIFOs TX et RX et les nettoyer
    LPC_UART0->FCR = 0x07;
}

void uart0_send_string(const char *str)
{
    // Configure P0.0 as GPIO output for a debug toggle (non-intrusive)
    LPC_PINCON->PINSEL0 &= ~(3 << 0); // P0.0 function = GPIO
    LPC_GPIO0->FIODIR |= (1 << 0);

    while (*str)
    {
        // Attendre que le registre de transmission soit vide (Flag THRE)
        while (!(LPC_UART0->LSR & 0x20));

        // Toggle P0.0 high to indicate a byte transmit start
        LPC_GPIO0->FIOSET = (1 << 0);

        // Transmettre le caractère
        LPC_UART0->THR = *str++;

        // Small software delay (couple cycles) then clear the toggle
        for (volatile int i = 0; i < 50; ++i) { __NOP(); }
        LPC_GPIO0->FIOCLR = (1 << 0);
    }
}

// Copie des initialisations principales pour tests/debug
void copy_main_init(void)
{
    // Initialisations Infrarouge (si présentes dans le projet)
    extern void init_PWM_IR(void);
    extern void init_Timer_Enveloppe(uint32_t);
    extern void init_microswitchs(void);
    extern void init_capteur_inductif(void);
    extern void set_debug_uart_enabled(uint8_t);

    init_PWM_IR();
    init_Timer_Enveloppe(250);

    // Initialisations UART et capteurs
    init_uart0();
    init_microswitchs();
    init_capteur_inductif();

    // SysTick rapide pour tests (100Hz)
    SysTick_Config(SystemCoreClock / 100);

    // Activer le debug UART localement
    set_debug_uart_enabled(1);
}

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
    // Utilisation identique à l'exemple fonctionnel : définir les bits sans les réinitialiser globalement
    LPC_PINCON->PINSEL0 |= (1 << 4) | (1 << 6);   // P0.2 TXD0, P0.3 RXD0

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

    // 7. Activer les FIFOs TX et RX et les nettoyer (mode simple)
    LPC_UART0->FCR = 0x01;

    // 8. Activer interruption RX Data Available (optionnel, utile pour debug)
    LPC_UART0->IER = 0x01;
    NVIC_EnableIRQ(UART0_IRQn);
}

void uart0_send_string(const char *str)
{
    while (*str)
    {
        // Attendre que le registre de transmission soit vide (Flag THRE)
        while (!(LPC_UART0->LSR & 0x20));

        // Transmettre le caractère
        LPC_UART0->THR = *str++;
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

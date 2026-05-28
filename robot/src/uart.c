#include "LPC17xx.h"
#include "uart.h"

void init_uart1(void)
{
    // 1. Allumer l'alimentation du bloc UART1
    LPC_SC->PCONP |= (1 << 4);

    // 2. Assurer que PCLK pour UART1 est réglé (on conserve la valeur par défaut)
    LPC_SC->PCLKSEL0 &= ~(3 << 8);

    // 3. Configurer les broches P0.15 en TXD1 et P0.16 en RXD1
    // P0.15 -> PINSEL0 bits [31:30] = 01
    LPC_PINCON->PINSEL0 &= ~(3UL << 30);
    LPC_PINCON->PINSEL0 |=  (1UL << 30);
    // P0.16 -> PINSEL1 bits [1:0] = 01
    LPC_PINCON->PINSEL1 &= ~0x3;
    LPC_PINCON->PINSEL1 |= 0x1;

    // 4. Configuration UART1 (8 bits de données, 1 bit d'arrêt, pas de parité)
    LPC_UART1->LCR = 0x83; // DLAB = 1 permet l'accès aux diviseurs de baudrate

    // 5. Calcul Baudrate = 115200. PCLK = 25 MHz (hypothèse)
    // Utilisation de la configuration équivalente au cas UART0
    LPC_UART1->DLL = 9;
    LPC_UART1->DLM = 0;
    LPC_UART1->FDR = (2 << 4) | 1;

    // 6. Désactiver DLAB (Verrouille le baudrate)
    LPC_UART1->LCR = 0x03;

    // 7. Activer les FIFOs TX et RX et les nettoyer
    LPC_UART1->FCR = 0x07;
}

void uart1_send_string(const char *str)
{
    while (*str)
    {
        // Attendre que le registre de transmission soit vide (Flag THRE)
        while (!(LPC_UART1->LSR & 0x20));

        // Transmettre le caractère
        LPC_UART1->THR = *str++;
    }
}

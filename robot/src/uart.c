#include "LPC17xx.h"
#include "uart.h"

void init_uart0(void)
{
    // 1. Allumer l'alimentation du bloc UART0
    LPC_SC->PCONP |= (1 << 3);

    // 2. Assurer que PCLK pour UART0 est CCLK/4 = 25 MHz (Valeur par défaut)
    LPC_SC->PCLKSEL0 &= ~(3 << 6);

    // 3. Configurer les broches P0.2 en TXD0 et P0.3 en RXD0
    // PINSEL0 bits [5:4] à 01, et bits [7:6] à 01
    LPC_PINCON->PINSEL0 &= ~0x000000F0;
    LPC_PINCON->PINSEL0 |=  0x00000050;

    // 4. Configuration UART (8 bits de données, 1 bit d'arrêt, pas de parité)
    LPC_UART0->LCR = 0x83; // DLAB = 1 permet l'accès aux diviseurs de baudrate

    // 5. Calcul Baudrate = 115200. PCLK = 25 MHz
    // Formule: PCLK / (16 * 115200) = 13.56
    // Utilisation de la configuration la plus standard :
    // DLL=9, DIVADDVAL=1, MULVAL=2 -> (1+1/2)=1.5
    // 25M / (16 * 9 * 1.5) = 115740 (Erreur de 0.4%, excellent)
    LPC_UART0->DLL = 9;
    LPC_UART0->DLM = 0;
    LPC_UART0->FDR = (2 << 4) | 1;

    // 6. Désactiver DLAB (Verrouille le baudrate)
    LPC_UART0->LCR = 0x03;

    // 7. Activer les FIFOs TX et RX et les nettoyer
    LPC_UART0->FCR = 0x07;
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

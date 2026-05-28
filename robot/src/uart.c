#include "LPC17xx.h"
#include "uart.h"

void init_uart3(void)
{
    // 1. Allumer l'alimentation du bloc UART3 (Bit 25 du registre PCONP)
    LPC_SC->PCONP |= (1 << 25);

    // 2. Assurer que PCLK pour UART3 est réglé par défaut (PCLKSEL1 bits 18 et 19)
    LPC_SC->PCLKSEL1 &= ~(3 << 18);

    // 3. Configurer les broches P0.0 en TXD3 et P0.1 en RXD3
    // P0.0 -> PINSEL0 bits [1:0] = 10 (binaire)
    // P0.1 -> PINSEL0 bits [3:2] = 10 (binaire)
    LPC_PINCON->PINSEL0 &= ~0x0F; // On efface les 4 premiers bits (bits 0 à 3)
    LPC_PINCON->PINSEL0 |=  0x0A; // On écrit 1010 en binaire (soit 0xA) pour activer la fonction alternative 2

    // 4. Configuration UART3 (8 bits de données, 1 bit d'arrêt, pas de parité)
    LPC_UART3->LCR = 0x83; // DLAB = 1 permet l'accès aux diviseurs de baudrate

    // 5. Calcul Baudrate = 115200 (Identique à ton code)
    LPC_UART3->DLL = 9;
    LPC_UART3->DLM = 0;
    LPC_UART3->FDR = (2 << 4) | 1;

    // 6. Désactiver DLAB (Verrouille le baudrate)
    LPC_UART3->LCR = 0x03;

    // 7. Activer les FIFOs TX et RX et les nettoyer
    LPC_UART3->FCR = 0x07;
}

void uart3_send_string(const char *str)
{
    while (*str)
    {
        // Attendre que le registre de transmission soit vide (Flag THRE)
        while (!(LPC_UART3->LSR & 0x20));

        // Transmettre le caractère
        LPC_UART3->THR = *str++;
    }
}
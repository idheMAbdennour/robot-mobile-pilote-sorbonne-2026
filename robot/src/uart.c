/**
 * @file uart.c
 * @brief Fichier du module uart.
 */

#include "uart.h"

#include "LPC17xx.h"

/* ==========================================================================
 * VARIABLES INTERNES
 * ========================================================================== */
#define UART_TX_BUFFER_SIZE 256
static char tx_buffer[UART_TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

void init_uart0(void) {
    uint32_t pclk;
    uint16_t dl;

    // 1. Alimenter UART0
    LPC_SC->PCONP |= (1 << 3);

    // 2. Horloge PCLK UART0 = CCLK / 4
    LPC_SC->PCLKSEL0 &= ~(3 << 6);

    // 3. Configurer P0.2 = TXD0, P0.3 = RXD0
    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |= (1 << 4) | (1 << 6); // Fonction 01 (TXD0/RXD0) sur bits 4:5 et 6:7

    // 4. Configuration UART : 8 bits, 1 stop, pas de parité
    // DLAB = 1 (pour accèder au diviseur)
    LPC_UART0->LCR = 0x83;

    // 5. Configuration Baudrate = 115200
    // PCLK = SystemCoreClock / 4. (Standard 25MHz)
    pclk = SystemCoreClock / 4;

    // On active le diviseur fractionnaire (FDR) = 1.5
    // MULVAL = 2, DIVADDVAL = 1  => FDR = (2 << 4) | 1 = 0x21
    // La formule devient : DL = PCLK / (16 * Baudrate * 1.5)
    // dl_float = (PCLK * 10) / (16 * Baudrate * 15)
    dl = (pclk * 10) / (16 * 115200 * 15);

    LPC_UART0->FDR = 0x21; // Configuration fractionnaire pour 115200 bauds
    LPC_UART0->DLM = (dl >> 8) & 0xFF;
    LPC_UART0->DLL = dl & 0xFF;

    // 6. DLAB = 0 (accès aux registres normaux)
    LPC_UART0->LCR = 0x03;

    // 7. Activer et réinitialiser les FIFOs
    LPC_UART0->FCR = 0x07;
}

void uart0_update(void) {
    // Si on a des données à envoyer ET que le registre THR est vide (prêt à transmettre)
    if (tx_head != tx_tail) {
        if (LPC_UART0->LSR & (1 << 5)) {
            LPC_UART0->THR = tx_buffer[tx_tail];
            tx_tail = (tx_tail + 1) % UART_TX_BUFFER_SIZE;
        }
    }
}

void uart0_send_char(char c) {
    uint16_t next_head = (tx_head + 1) % UART_TX_BUFFER_SIZE;
    
    // Si le buffer est plein, on force le vidage actif pour ne pas perdre la donnée
    while (next_head == tx_tail) {
        uart0_update();
    }
    
    tx_buffer[tx_head] = c;
    tx_head = next_head;
}

void uart0_send_string(const char *str) {
    while (*str) {
        uart0_send_char(*str++);
    }
}

void uart0_send_frame(const char *str) {
    // Alias pour envoyer une chaîne complète
    uart0_send_string(str);
}

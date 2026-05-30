/**
 * @file uart.h
 * @brief Fichier du module uart.
 */

#ifndef UART_H
#define UART_H

/* ==========================================================================
 * DÉFINITIONS (PINS)
 * ========================================================================== */
#define PIN_UART0_TX (1 << 2) // P0.2 - Bits 4:5 de PINSEL0
#define PIN_UART0_RX (1 << 3) // P0.3 - Bits 6:7 de PINSEL0

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le périphérique UART0 à 115200 bauds.
 */
void init_uart0(void);

/**
 * @brief Envoie un caractère unique sur UART0.
 * @param c Le caractère à envoyer.
 */
void uart0_send_char(char c);

/**
 * @brief Envoie une chaîne de caractères sur UART0.
 * @param str Pointeur vers la chaîne terminée par null.
 */
void uart0_send_string(const char *str);

/**
 * @brief Envoie une trame (alias de uart0_send_string pour cohérence).
 * @param str Pointeur vers la chaîne de caractères.
 */
void uart0_send_frame(const char *str);

#endif // UART_H

#ifndef UART_H
#define UART_H

void init_uart0(void);
void uart0_send_string(const char *str);
// Reused init (copie du main) pour tests/debug
void copy_main_init(void);

#endif // UART_H

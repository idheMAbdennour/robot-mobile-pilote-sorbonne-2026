#ifndef UART_H
#define UART_H

void init_uart0(void);
void uart0_send_string(const char *str);
void uart0_send_frame(const char *str);

#endif // UART_H

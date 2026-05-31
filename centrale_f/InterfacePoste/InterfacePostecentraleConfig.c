#include "InterfacePostecentraleConfig.h"

void init_uart3_centrale(void) {
    // P0.0 et P0.1 -> TXD3 et RXD3
    LPC_SC->PCONP |= (1 << 25);
    LPC_SC->PCLKSEL1 &= ~(3 << 18);
	
    LPC_PINCON->PINSEL0 &= ~0xF;
    LPC_PINCON->PINSEL0 |= (2 << 0) | (2 << 2);
    LPC_PINCON->PINMODE0 &= ~(3 << 2);

    LPC_UART3->LCR = (3 << 0) | (1 << 7);
    LPC_UART3->DLL = 0xA3;
    LPC_UART3->DLM = 0x00;
    LPC_UART3->LCR &= ~(1 << 7);

    LPC_UART3->FCR = (1 << 0) | (1 << 1) | (1 << 2);
   
    LPC_UART3->IER |= (1 << 0); 
    
    NVIC_EnableIRQ(UART3_IRQn);
}

void init_leds(void) {
    LPC_GPIO3->FIODIR |= (1 << 25) | (1 << 26);
    LPC_GPIO0->FIODIR |= (1 << 22);
   
    LPC_GPIO3->FIOSET = (1 << 25) | (1 << 26);
    LPC_GPIO0->FIOSET = (1 << 22);
}

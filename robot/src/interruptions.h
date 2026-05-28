#ifndef INTERRUPTIONS_H
#define INTERRUPTIONS_H

#include "LPC17xx.h"

// Prototypes des handlers système qui seront définis dans interruptions.c
void EINT3_IRQHandler(void);
void EINT1_IRQHandler(void);
void TIMER0_IRQHandler(void);

#endif /* INTERRUPTIONS_H */
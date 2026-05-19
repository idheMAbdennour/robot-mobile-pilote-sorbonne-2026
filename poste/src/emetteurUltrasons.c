#include "emetteurUltrasons.h"

/* 
	coter char -> 'G' ou 'D'
	numPoste -> entier
*/
void initUltrasonsPWM(int numPoste, char coter) {
	// PWM1.1
	LPC_PINCON->PINSEL4 |= 1 << 0;
	LPC_PINCON->PINSEL4 &= ~(1 << 1);
	
	// Reset on MR0
	LPC_PWM1->MCR |= 1 << 1;
	
	// Numéro de poste dans MR0
	LPC_PWM1->MR0 = 200 + 500 + 200 * numPoste;
	
	// Gauche ou droite dans MR1
	if (coter == 'G') {
		LPC_PWM1->MR1 = 200;
	} else {
		LPC_PWM1->MR1 = 300;
	}
	
	// Confirm change MR0 et MR1
	LPC_PWM1->LER |= 1 << 0;
	LPC_PWM1->LER |= 1 << 1;
	
	// Enable & Reset
	LPC_PWM1->TCR |= 1 << 0 | 1 << 1 | 1 << 3;
	LPC_PWM1->PCR |= 1 << 0;
	LPC_PWM1->TCR &= ~(1 << 1);
}


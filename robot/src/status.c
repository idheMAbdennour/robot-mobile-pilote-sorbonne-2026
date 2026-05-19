#include "status.h"

void delay(int msec) {
	volatile uint16_t count = 0;
	for (int i=0; i<msec * 1000000; i++) {
		count++;
	}
}

// Initialise la LED RVB pour l'affichage du status du robot
void initLedChangementStatus() {
	LPC_PINCON->PINSEL2 &= ~(3 << 22); // GPIO P1.27 -> Rouge
	LPC_PINCON->PINSEL2 &= ~(3 << 24); // GPIO P1.28 -> Vert
	LPC_PINCON->PINSEL2 &= ~(3 << 26); // GPIO P1.29 -> Bleu

	// GPIOs en sortie
	LPC_GPIO1->FIODIR |= 1 << 27;
	LPC_GPIO1->FIODIR |= 1 << 28;
	LPC_GPIO1->FIODIR |= 1 << 29;

	// GPIOs à 0 par default
	LPC_GPIO1->FIOCLR |= 1 << 27;
	LPC_GPIO1->FIOCLR |= 1 << 28;
	LPC_GPIO1->FIOCLR |= 1 << 29;
}

// À appeller quand le status du robot change
void changementDeStatus(char nouveauStatus) {
	// Reset tous
	LPC_GPIO1->FIOCLR |= 1 << 27;
	LPC_GPIO1->FIOCLR |= 1 << 28;
	LPC_GPIO1->FIOCLR |= 1 << 29;

	switch(nouveauStatus) {
	case 'L':
		LPC_GPIO1->FIOSET |= 1 << 28; // Vert
	break;
	case 'E':
		LPC_GPIO1->FIOSET |= 1 << 27; // Rouge
	break;
	case 'C':
		LPC_GPIO1->FIOSET |= 1 << 27; // Rouge et blue
		LPC_GPIO1->FIOSET |= 1 << 29; // Rouge et blue
	break;
	case 'D':
		LPC_GPIO1->FIOSET |= 1 << 29; // Bleu
	break;
	}
}

int mainTestStatusLED() {
	changementDeStatus('L');
	
	while(1) {
		// Teste status
		changementDeStatus('L');
		delay(5);
		changementDeStatus('E');
		delay(5);
		changementDeStatus('C');
		delay(5);
		changementDeStatus('D');
		delay(5);
	}
}

#include "BeeperMethods.h"

volatile int mSec = 1250;
volatile int freqAsChanged = 0;
volatile int nbFoisGlo = 0;

#define GPIO_BEEP LPC_GPIO2
#define GPIO_BEEP_NUM 4

void stopBeep() {
	GPIO_BEEP->FIOCLR = 1 << GPIO_BEEP_NUM;
	
	LPC_TIM1->TCR &= ~(1 << 0); // Enable
	LPC_TIM1->TCR |= 1 << 1; // Reset
	nbFoisGlo = 0;
}

void changeDist(int msecVal) {
	freqAsChanged = 1;
	mSec = msecVal;
}

void freqChanged() {
		LPC_TIM1->MR0 = 100000 * mSec / 4;
}

void startBeepChargeDecharge(int nbFois) {
	// Start Timer
	nbFoisGlo = nbFois;
	if (mSec != 1250) {
		mSec = 1250;
		freqChanged();
	}
	
	LPC_TIM1->MR1 = 100000 * 750 / 4;
	GPIO_BEEP->FIOSET = 1 << GPIO_BEEP_NUM;
	
	LPC_TIM1->TCR &= ~(1 << 1); // Reset
	LPC_TIM1->TCR |= 1 << 0; // Enable
}

void startBeep(int msecVal) {
	// Start Timer
	nbFoisGlo = -1;
	mSec = msecVal;
	freqChanged();
	
	LPC_TIM1->MR1 = 100000 * 250 / 4;
	GPIO_BEEP->FIOSET = 1 << GPIO_BEEP_NUM;
	
	LPC_TIM1->TCR &= ~(1 << 1); // Reset
	LPC_TIM1->TCR |= 1 << 0; // Enable
}

void initBeepeur() {
	// Configurer P2.4 en tant que GPIO
	LPC_PINCON->PINSEL4 &= ~(3 << 8);

	// GPIO
	GPIO_BEEP->FIODIR |= 1 << GPIO_BEEP_NUM;
	GPIO_BEEP->FIOCLR = 1 << GPIO_BEEP_NUM;
	
	// TIMER1
	freqChanged();
	LPC_TIM1->MCR |= 3 << 0;
	
	LPC_TIM1->MR1 = 100000 * 750 / 4;
	LPC_TIM1->MCR |= 1 << 3;
	
	NVIC_EnableIRQ(TIMER1_IRQn);
	LPC_TIM1->TCR |= 1 << 1; // Reset
}

void TIMER1_IRQHandler() {
	// MR1
	if (LPC_TIM1->IR & 1<<1) {
		LPC_TIM1->IR |= 1 << 1;
		
		GPIO_BEEP->FIOCLR = 1 << GPIO_BEEP_NUM;
	}
	
	// MR0
	if (LPC_TIM1->IR & 1<<0) {
		LPC_TIM1->IR |= 1 << 0;
		
		if (freqAsChanged == 1) {
			freqChanged();
		}
		
		nbFoisGlo = nbFoisGlo - 1;
		if (nbFoisGlo == 0) {
			LPC_TIM1->TCR &= ~(1 << 0); // Enable
			LPC_TIM1->TCR |= 1 << 1; // Reset
			GPIO_BEEP->FIOCLR = 1 << GPIO_BEEP_NUM;
		} else {
			GPIO_BEEP->FIOSET = 1 << GPIO_BEEP_NUM;
		}
	}
}

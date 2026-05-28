#include "recepSPI.h"

int bufferVal[8];
int bufferPos = 0;

uint8_t lastCS[8] = {-1,-1,-1,-1,-1,-1, -1, -1};
uint8_t posWriteCS = 0;
uint8_t posReadCS = 0;

uint8_t Vg, Vd, Pg, Pd;

// Interuption horlage
void initClk() {
	// Ext 1 Interupt P2.11
	LPC_PINCON->PINSEL4 &= ~(1 << 23);
	LPC_PINCON->PINSEL4 |= 1 << 22;
	
	// Front montant
	LPC_SC->EXTMODE |= 1 << 1;
	LPC_SC->EXTPOLAR |= 1 << 1;
	
	NVIC_EnableIRQ(EINT1_IRQn);
}

void initMISO() {
	// GPIO IN P0.8
	LPC_PINCON->PINSEL0 &= ~(3 << 16);
	LPC_GPIO0->FIODIR &= ~(1 << 8);
}

void initCS1234() {
	// GPIO OUT P0.6 P0.7 P0.9 P0.10
	LPC_PINCON->PINSEL0 &= ~(3 << 12);
	LPC_PINCON->PINSEL0 &= ~(3 << 14);
	LPC_PINCON->PINSEL0 &= ~(3 << 18);
	LPC_PINCON->PINSEL0 &= ~(3 << 20);
	
	LPC_GPIO0->FIODIR |= 1 << 6;
	LPC_GPIO0->FIODIR |= 1 << 7;
	LPC_GPIO0->FIODIR |= 1 << 9;
	LPC_GPIO0->FIODIR |= 1 << 10;
	
	LPC_GPIO0->FIOCLR |= 1 << 6;
	LPC_GPIO0->FIOCLR |= 1 << 7;
	LPC_GPIO0->FIOCLR |= 1 << 9;
	LPC_GPIO0->FIOCLR |= 1 << 10;
}

void setCSVal(int csNum) {
	// Don't erase askings
	if ((posWriteCS + 1) % 8 == posReadCS)
		return;
	
	// New asking
	if (csNum == 0) {
		LPC_GPIO0->FIOSET |= 1 << 6;
		lastCS[posWriteCS] = 0;
	} else if (csNum == 1) {
		LPC_GPIO0->FIOSET |= 1 << 7;
		lastCS[posWriteCS] = 1;
	} else if (csNum == 2) {
		LPC_GPIO0->FIOSET |= 1 << 9;
		lastCS[posWriteCS] = 2;
	} else if (csNum == 3) {
		LPC_GPIO0->FIOSET |= 1 << 10;
		lastCS[posWriteCS] = 3;
	}
	posWriteCS = (posWriteCS + 1) % 8;
}

void recepSPI_interrupt_routine(void) {
	// FLAG
	LPC_SC->EXTINT |= 1 << 1;
	
	// RESET all CS
	LPC_GPIO0->FIOCLR |= 1 << 6;
	LPC_GPIO0->FIOCLR |= 1 << 7;
	LPC_GPIO0->FIOCLR |= 1 << 9;
	LPC_GPIO0->FIOCLR |= 1 << 10;
	
	// Read if should
	if (posWriteCS == posReadCS)
		return;
	
	// Read MISO
	int val = (LPC_GPIO0->FIOPIN & (1 << 8)) > 0;
	
	bufferVal[bufferPos] = val;
	bufferPos++;
	
	if (bufferPos == 8) {
		bufferPos = 0;
		
		// Decode la valeur obtenue
		int decodedVal = bufferVal[0] + 2 * (bufferVal[1] + 2 * (bufferVal[2] + 2 * (bufferVal[3] + 2 * (bufferVal[4] + 2 * (bufferVal[5]+ 2 * (bufferVal[6]+ 2 * (bufferVal[7])))))));
		if (lastCS[posReadCS] == 0)
			Vg = decodedVal;
		else if (lastCS[posReadCS] == 1)
			Vd = decodedVal;
		else if (lastCS[posReadCS] == 2)
			Pg = decodedVal;
		else if (lastCS[posReadCS] == 3)
			Pd = decodedVal;
		
		posReadCS = (posReadCS + 1 ) % 8;
	}
}
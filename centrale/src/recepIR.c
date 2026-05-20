#include "recepIR.h"

// Var global
int numeroRobot = -1;
int vitesseRobot = -1;
int statusRobot = -1;


char estValide = 0;
char state = 0; // 0 -> entete, 1 numRobot, 2 vitesse, 3 status, 4 checksum

char bitNb = 0;
char bitObtenus[4];

int numRobotTemp = 0;
int vitesseTemp = 0;
int statusTemp = 0;
int checksumTemp = 0;

char buffer[10];
char bufferIndex = 0;

int captures[2] = {0, 0};

// Init le timer 0 avec au tau donc MR0 de 200 x 10e+6
void initTimerRecepIR() {
	// Timer 0.1
	LPC_TIM0->MR0 = 200000000;
	
	// Interupt & reset on MR0
	LPC_TIM0->MCR |= 1 << 0; 
	LPC_TIM0->MCR |= 1 << 1; 
	
	//Enable IRQ
	NVIC_EnableIRQ(TIMER0_IRQn);
	
	// Reset & Enable
	LPC_TIM0->TCR  |= 1 >> 1;
	LPC_TIM0->TCR  |= 1 >> 0;
	LPC_TIM0->TCR  &= ~(1 >> 1);
}


// Doit interompre sur front montant et descendant
void initGPIOInteruptRecepIR() {	
	// IR Gauche GPIO P0.21
	LPC_PINCON->PINSEL1 &= ~(3 << 10);
	LPC_GPIOINT->IO0IntEnR |= 1 << 21;
	LPC_GPIOINT->IO0IntEnF |= 1 << 21;
	
	// IR Droite GPIO P0.22
	//LPC_PINCON->PINSEL1 &= ~(3 << 12);
	//LPC_GPIOINT->IO0IntEnR |= 1 << 22;
	//LPC_GPIOINT->IO0IntEnF |= 1 << 22;
}


// Gestion du changement de tau si besoin
void changeTimerTauRecepIR(int newTau) {
	// Change MR0 + devalide la trame
	if (newTau < 150)
		return;
	
	if (newTau > 300)
		return;
	
	int diffTau = newTau - LPC_TIM0->MR0;
	
	// "Intervale de confiance"
	if (diffTau > 10 || diffTau < 10) {
		estValide = 0;
		state = 0;
	}
	
	// Reset le timer si nouveau tau < val actuel (en deux temps)
	if ( captures[1] > newTau )
		LPC_TIM0->TCR  |= 1 >> 1;
	
	// Mettre le nouveau tau
	LPC_TIM0->MR0 = newTau; 
	
	// Reset le timer si nouveau tau < val actuel (deuxième temps)
	if ( captures[1] > newTau )
		LPC_TIM0->TCR  &= ~(1 >> 1);
}


// Interuption GPIO -> Appelle cette fonction
void interuptGPIORecepIR () {
	LPC_GPIOINT->IO0IntClr |= 1 << 21;
	
	// Capture A verif le fonctionnement de capture register
	
	captures[0] = captures[1];
	captures[1] = LPC_TIM0->CR0;

	// Calcule du tau
		int newTau = 0;
	if (captures[1] > captures[0]) {
		newTau = captures[1] - captures[0];
		
	} else { // captures[1] < captures[0]
		newTau = (LPC_TIM0->MR0 - captures[0]) + (captures[1] - 0);
		
	}
		
	// le met dans MR0
	changeTimerTauRecepIR(newTau);
}

// Reception + Decodage
void TIM0_IRQHandler() {
	// 101 -> entete 10
	// 110 -> recoit 1
	// 100 -> recoit 0
	
	buffer[bufferIndex] = (LPC_GPIO0->FIOPIN & (1 << 21)) > 0;
	
	// Cherche l'entete
	if (state == 0) {
		if (buffer[bufferIndex] == 1
		&& buffer[(bufferIndex-1) % 10] == 0
		&& buffer[(bufferIndex-2) % 10] == 1) {

			estValide = 1;
			state = 1;
			bitNb = 0;
			
			buffer[0] = buffer[bufferIndex];
			buffer[1] = 0;
			buffer[2] = 0;
			bufferIndex = 0;
		}
	}
	
	bufferIndex = (bufferIndex + 1) % 10;
	
	// Cherche pas l'entete
	if (state != 0) {
		if (bufferIndex == 3) {
			// Detecte 0
			if (buffer[0] == 1
			&& buffer[1] == 0
			&& buffer[2] == 0) {
				bitObtenus[bitNb++] = 0;
				
			}
			
			// Detecte 1
			if (buffer[0] == 1
			&& buffer[1] == 1
			&& buffer[2] == 0) {
				bitObtenus[bitNb++] = 1;
				
			}
		}
	}
	
	if (bitNb == 4) {
		bitNb = 0;
		
		int valTemp = bitObtenus[0] + 2 * (bitObtenus[1] + 2 * (bitObtenus[2] + 2 * bitObtenus[3]) );
		
		if (state == 1)
			numRobotTemp = valTemp;
		else if (state == 2)
			vitesseTemp = valTemp;
		else if (state == 3)
			statusTemp = valTemp;
		else if (state == 4) {
			checksumTemp = valTemp;
			
			// Verif du checksum
			int sum4bits = (numRobotTemp + vitesseTemp + statusTemp) & 0x0F;	// Calcul de la somme sur 4 bits des 3 quartets d'informations
			
			if(checksumTemp == ((sum4bits ^ 0x0F) + 1)){ 		// Comparaison du complément à 2 avec le checksum	 
				if (estValide == 1)
					numeroRobot = numRobotTemp;
					vitesseRobot = vitesseTemp;
					statusRobot = statusTemp;
					
				}
			}
		}
		
		state = (state + 1) % 5;
	}
	

}


// Temp
void EXTINT3_IRQHandler() {
	interuptGPIORecepIR();
}
#include "emissionIR.h"
#include <stdint.h>

#include "LPC17xx.h"
#include "robotState.h"

// Timer0 pour gérer l'enveloppe de modulation IR (250us = un temps 't')
#define TIMER_PCLK (25000000) // Si l'horloge periphérique est à 25MHz
#define TIMER_PRESCALER 25 // Pour obtenir une résolution de 1us


// PWM 38kHz sur P1.22
#define PCLK_PWM (25000000) // Si l'horloge periphérique est à 25MHz
#define FREQ_IR 38000
#define PWM_PRESCALER 25 // Pour obtenir une résolution de 1us
#define PWM_PERIOD (PCLK_PWM / (PWM_PRESCALER * FREQ_IR)) // Nombre de ticks pour 38kHz

// Broche P1.22
#define PIN_MASK (1 << 22)

void init_PWM_IR(void) {
    // 1. Définir le P1.22 en mode PWM1.3
	LPC_PINCON->PINSEL3 &= ~(3 << 12); // Clear bits pour P1.22
	LPC_PINCON->PINSEL3 |= (2 << 12);  // Fixer P1.22 à la fonction PWM1.3 (0b10)

	// 2. Configurer le PWM1.3 pour une fréquence de 38kHz
	LPC_SC->PCLKSEL0 &= ~(3 << 12); // PCLK_PWM1 = CCLK (25MHz)
	LPC_PWM1->PR = PWM_PRESCALER - 1; // Prescaler pour 1us

	// 3. PWM1.3 en mode single edge
	LPC_PWM1->PCR &= ~(1 << 3); // Activer PWM1.3 en single edge

	// 3. Définir la période (MR0) et le rapport cyclique pour 50% (MR3)
	LPC_PWM1->MR0 = PWM_PERIOD - 1; // Période pour 38kHz
	LPC_PWM1->MR3 = PWM_PERIOD / 2 - 1; // Rapport cyclique pour 50%

	// 4. Latch des registres MR0 et MR3
	LPC_PWM1->LER = (1 << 0) | (1 << 3);

	// // 5. Comportement lorsque le Timer atteint MR0 (période) : Reset et coninuer
	// LPC_PWM1->MCR &= ~(1 << 0); // Pas d'interruption sur MR0
	// LPC_PWM1->MCR |= (1 << 1); // Reset on MR0
	// LPC_PWM1->MCR &= ~(1 << 2); // Pas de stop sur MR0

	// // 6. Configurer la sortie activée, mais compteur à l'arrêt et reseté par défaut
    // LPC_PWM1->PCR |= (1 << 11);          // Enable PWM3 output (reste connectée à P1.22)


	// LPC_PWM1->TCR &= ~(1 << 0); // Counter OFF (0)
	// LPC_PWM1->TCR |= (1 << 1) | (1 << 3); // Reset ON (1), PWM Mode (3)
	// LPC_PWM1->TCR &= ~(1 << 1); // Reset OFF (0)

    // 5. Comportement lorsque le Timer atteint MR0 (période) : Reset et coninuer
    LPC_PWM1->MCR &= ~(1 << 0);
    LPC_PWM1->MCR |= (1 << 1);
    LPC_PWM1->MCR &= ~(1 << 2);

    // 6. Configurer la sortie activée, mais compteur à l'arrêt et reseté par défaut
    LPC_PWM1->PCR &= ~(1 << 11);

    // 6. On active le mode PWM et on laisse le compteur tourner librement
    LPC_PWM1->TCR = (1 << 3) | (1 << 0);
}

void init_Timer_Enveloppe(uint16_t délai_us) {
    // 1. Allumer le Timer 0
    LPC_SC->PCONP |= (1 << 1);

    // 2. Horloge pour Timer 0 (CCLK/4 -> 25MHz)
    LPC_SC->PCLKSEL0 &= ~(3 << 2);

    // 3. Configuration du Timer pour 250us (un temps 't')
    LPC_TIM0->PR = TIMER_PRESCALER - 1; // Prescaler pour obtenir 1us
    LPC_TIM0->MR0 = délai_us - 1; // ticks = délai_us

    // 4. Reset & Interrupt sur Match 0
    LPC_TIM0->MCR |= (1 << 0) | (1 << 1);
	LPC_TIM0->TCR &= ~(1 << 2); // Clear stop

    // 5. Activer l'interruption au niveau du NVIC
    NVIC_EnableIRQ(TIMER0_IRQn);
}


// Interruption appelée toutes les 250us (depuis un temps 't') par interruptions.c
void emissionIR_interrupt_routine(void) {
    // Acquitter l'interruption
    LPC_TIM0->IR = 1;

    // Gérer l'état de l'émission
    update_PWM_state();
}



// Sous-routine pour gérer l'état du matériel PWM à chaque temps 't'
void update_PWM_state(void) {

    // PHASE BLANC LORS DES CYCLES 3, 4 et 5 (Silence entre les rafales)
    if (frame_counter >= 3) {
        LPC_PWM1->PCR &= ~(1 << 11); // Désactiver la sortie -> 0V

        seq_index++;
        if (seq_index >= seq_length) {
            seq_index = 0;
            frame_counter++;
            if (frame_counter >= 6) {
                frame_counter = 0; // On reprend un nouveau cycle d'émission, reconstruire la trame :
                preparer_trame(get_robot_number(), get_robot_vitesse(), (uint8_t)get_robot_status());
            }
        }
        return;
    }

    // PHASE D'ÉMISSION LORS DES CYCLES 0, 1, 2
if (ir_sequence[seq_index] == 1) {
        // Reset le compteur pour synchroniser la phase à 38kHz
        LPC_PWM1->TCR |= (1 << 1);  // Reset ON
        LPC_PWM1->TCR &= ~(1 << 1); // Reset OFF

        // Activer la sortie
        LPC_PWM1->PCR |= (1 << 11);
    } else {
        // Blanc : Désactiver la sortie
        LPC_PWM1->PCR &= ~(1 << 11);
    }

    // Passage au temps 't' suivant
    seq_index++;
    if (seq_index >= seq_length) {
        // Une trame est complétement transmise
        seq_index = 0;
        frame_counter++;

		LPC_PWM1->PCR &= ~(1 << 11);
    }
}


void mainTestEmissionIR() {
	// Test de préparation d'une trame IR
	preparer_trame(0x0A, 0x05, 0x03); // id:10, vitesse:5 (soit 25%), status:3 (0011)
}

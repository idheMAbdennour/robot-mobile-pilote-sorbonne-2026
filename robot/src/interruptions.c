#include "interruptions.h"

// Inclusions des composants qui fournissent leurs propres routines d'interruption
#include "dtmf.h"
#include "moteurs.h"
#include "capteurInductif.h"
#include "proximetre.h"
#include "emissionIR.h"
#include "recepSPI.h"

extern volatile uint8_t flag_50hz;

// ==============================================================================
// GESTION DU SYSTICK - Utilisé pour cadence le main à 50Hz
// ==============================================================================
void SysTick_Handler(void)
{
    flag_50hz = 1;
}

// ==============================================================================
// GESTION DES INTERRUPTIONS EXTERNES PARTAGEES SUR LE PORT 0 ET PORT 2 (EINT3)
// ==============================================================================
void EINT3_IRQHandler(void)
{
    // --- Routine DTMF (P0.20) ---
    dtmf_interrupt_routine();

    moteurs_interrupt_routine();
    capteurInductif_interrupt_routine();
    proximetre_interrupt_routine();
}

// ==============================================================================
// GESTION DES INTERRUPTIONS EXTERNES DEDIEES (EINT1) - Utilisé par recepSPI
// ==============================================================================
void EINT1_IRQHandler(void)
{
    // --- Routine de réception SPI ---
    recepSPI_interrupt_routine();
}

// ==============================================================================
// GESTION DES INTERRUPTIONS TIMER 0 - Utilisé par emissionIR
// ==============================================================================
void TIMER0_IRQHandler(void)
{
    // --- Routine d'émission IR ---
    emissionIR_interrupt_routine();
}

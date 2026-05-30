/**
 * @file interruptions.c
 * @brief Fichier du module interruptions.
 */

#include "interruptions.h"

#include "buttons.h"
#include "capteur_inductif.h"
#include "dtmf.h"
#include "emission_ir.h"
#include "moteur.h"
#include "proximetre.h"
#include "recep_spi.h"
#include "robot_state.h"

/* ==========================================================================
 * IMPLÉMENTATION DES HANDLERS D'INTERRUPTIONS
 * ========================================================================== */

/**
 * @brief Gestion du SysTick - Cadence la boucle principale (ex: 50Hz)
 */
void SysTick_Handler(void) {
    set_flag_50hz(1);
}

/**
 * @brief Gestion des interruptions externes partagées sur les ports 0 et 2
 */
void EINT3_IRQHandler(void) {
    // --- Routine DTMF (P0.20) ---
    dtmf_interrupt_routine();

    // --- Routine Moteurs (Codeurs) ---
    moteurs_interrupt_routine();

    // --- Routine Capteur Inductif (Switchs et Enveloppe) ---
    capteur_inductif_interrupt_routine();


    // --- Routine Boutons (Charge/Décharge) ---
    buttons_interrupt_routine();
}

/**
 * @brief Gestion des interruptions externes dédiées EINT1
 */
void EINT1_IRQHandler(void) {
    // --- Routine de réception SPI (Horloge) ---
    recep_spi_interrupt_routine();
}

/**
 * @brief Gestion des interruptions Timer 0
 */
void TIMER0_IRQHandler(void) {
    // --- Routine d'émission IR (Machine d'état) ---
    emission_ir_interrupt_routine();
}

/**
 * @brief Gestion des interruptions Timer 3
 */
void TIMER3_IRQHandler(void) {
    // --- Génération du signal Servo PWM logiciel (Proximètre) ---
    proximetre_timer_interrupt_routine();
}

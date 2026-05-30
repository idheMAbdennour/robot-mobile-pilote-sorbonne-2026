/**
 * @file pwm.c
 * @brief Fichier du module pwm.
 */

#include "pwm.h"

/* ==========================================================================
 * DÉFINITIONS PRIVÉES
 * ========================================================================== */
// Les définitions de PCLK_PWM, PWM_BASE_FREQ_HZ et PWM_MR0_PERIOD 
// se trouvent désormais dans pwm.h pour être partagées.

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

// --- PWM MOTEURS (PWM1.1 et PWM1.2) ---
void pwm_init_moteurs(void) {
    // 1. Activation de l'alimentation du bloc PWM1
    LPC_SC->PCONP |= (1 << 6);

    // 2. Configuration de l'horloge du PWM1 (PCLK_PWM1 = CCLK/4 = 25 MHz)
    LPC_SC->PCLKSEL0 &= ~(3 << 12);

    // 3. Configuration des broches P2.0 et P2.1 en mode PWM1.1 et PWM1.2
    LPC_PINCON->PINSEL4 &= ~0xF;   
    LPC_PINCON->PINSEL4 |= 0x5;    

    // 4. Configuration du compteur (Prescaler = 0)
    LPC_PWM1->PR = 0;
    LPC_PWM1->MR0 = PWM_MR0_PERIOD;

    // 5. Extinction initiale
    LPC_PWM1->MR1 = 0;
    LPC_PWM1->MR2 = 0;

    // 6. Reset sur Match 0 (Bouclage du timer sur la période MR0)
    LPC_PWM1->MCR &= ~(1 << 0);
    LPC_PWM1->MCR |= (1 << 1);
    LPC_PWM1->MCR &= ~(1 << 2);

    // 7. Demande de prise en compte (Latch)
    LPC_PWM1->LER |= (1 << 0) | (1 << 1) | (1 << 2);

    // 8. Activation des sorties PWM1.1 et PWM1.2
    LPC_PWM1->PCR |= (1 << 9) | (1 << 10);

    // 9. Démarrage
    LPC_PWM1->TCR = (1 << 0) | (1 << 3);
}

void pwm_set_moteur_gauche(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;
    LPC_PWM1->MR1 = (PWM_MR0_PERIOD * pourcent) / 100;
    LPC_PWM1->LER |= (1 << 1);
}

void pwm_set_moteur_droit(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;
    LPC_PWM1->MR2 = (PWM_MR0_PERIOD * pourcent) / 100;
    LPC_PWM1->LER |= (1 << 2);
}

void pwm_set_moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite) {
    if (pourcent_gauche > 100) pourcent_gauche = 100;
    if (pourcent_droite > 100) pourcent_droite = 100;

    LPC_PWM1->MR1 = (PWM_MR0_PERIOD * pourcent_gauche) / 100;
    LPC_PWM1->MR2 = (PWM_MR0_PERIOD * pourcent_droite) / 100;

    LPC_PWM1->LER |= (1 << 1) | (1 << 2);
}

// --- PWM IR (PWM1.3) ---
void pwm_init_ir(void) {
    // Note: P2.2 est configuré en mode PWM1.3 par emission_ir.c

    // 1. Activation de l'alimentation du bloc PWM1 (au cas où)
    LPC_SC->PCONP |= (1 << 6);

    // 2. Configurer l'horloge du PWM1 (PCLK_PWM1 = CCLK/4 = 25MHz)
    LPC_SC->PCLKSEL0 &= ~(3 << 12);
    LPC_PWM1->PR = 0;

    // 3. PWM1.3 en mode single edge
    LPC_PWM1->PCR &= ~(1 << 3);

    // 4. Période globale et rapport cyclique (50% pour IR)
    LPC_PWM1->MR0 = PWM_MR0_PERIOD;
    LPC_PWM1->MR3 = PWM_MR0_PERIOD / 2;

    // 5. Reset sur Match 0
    LPC_PWM1->MCR &= ~(1 << 0);
    LPC_PWM1->MCR |= (1 << 1);
    LPC_PWM1->MCR &= ~(1 << 2);

    // 6. Latch
    LPC_PWM1->LER |= (1 << 0) | (1 << 3);

    // 7. Sortie désactivée par défaut
    LPC_PWM1->PCR &= ~(1 << 11);

    // 8. Démarrer
    LPC_PWM1->TCR = (1 << 3) | (1 << 0);
}

void pwm_enable_ir_output(void) {
    LPC_PWM1->PCR |= (1 << 11);
}

void pwm_disable_ir_output(void) {
    LPC_PWM1->PCR &= ~(1 << 11);
}

void pwm_reset_counter_ir(void) {
    LPC_PWM1->TCR |= (1 << 1);  // Reset ON
    LPC_PWM1->TCR &= ~(1 << 1); // Reset OFF
}

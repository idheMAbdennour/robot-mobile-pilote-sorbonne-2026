/**
 * @file emission_ir.c
 * @brief Fichier du module emission_ir.
 */

#include "emission_ir.h"

#include "pwm.h"
#include "robot_state.h"
#include "timers.h"

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void update_pwm_state(void);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

void init_pwm_ir(void) {
    pwm_init_ir();

    // Configuration de la broche de synchro (P2.3) en sortie
    LPC_PINCON->PINSEL4 &= ~(3 << 6);
    LPC_GPIO2->FIODIR |= PIN_SYNC_IR;
    LPC_GPIO2->FIOCLR = PIN_SYNC_IR;
}

void init_timer_enveloppe(uint16_t delai_us) {
    timer0_init(delai_us);
}

void preparer_trame(uint8_t id, uint8_t vitesse, uint8_t status) {
    // Sécurité: sortie anticipée si les valeurs dépassent 4 bits (> 15)
    if (id > 0x0F || vitesse > 0x0F || status > 0x0F) {
        return;
    }

    // Calcul du checksum sur 4 bits (complément à 2)
    uint8_t somme = (id + vitesse + status) & 0x0F;
    uint8_t checksum = ((~somme) + 1) & 0x0F;

    // Payload concaténé sur 16 bits
    uint16_t payload = (id << 12) | (vitesse << 8) | (status << 4) | checksum;

    uint8_t idx = 0;

    // 1. Bit de Start : 1 impulsions, 1 blanc (10)
    set_ir_sequence_at(idx++, 1);
    set_ir_sequence_at(idx++, 0);

    // 2. Traitement des 16 bits de données du MSB vers le LSB
    for (int i = 15; i >= 0; i--) {
        uint8_t bit = (payload >> i) & 1;
        if (bit == 1) {
            // '1' logique = "110" (2t impulsions, 1t blanc)
            set_ir_sequence_at(idx++, 1);
            set_ir_sequence_at(idx++, 1);
            set_ir_sequence_at(idx++, 0);
        } else {
            // '0' logique = "100" (1t impulsions, 2t blanc)
            set_ir_sequence_at(idx++, 1);
            set_ir_sequence_at(idx++, 0);
            set_ir_sequence_at(idx++, 0);
        }
    }

    // 3. Lancement de la machine d'état (Interrupt Timer)
    set_ir_seq_length(idx);
    set_ir_seq_index(0);

    // Lever la patte de synchro au début de la trame (entête)
    LPC_GPIO2->FIOSET = PIN_SYNC_IR;

    // Démarrer le timer d'enveloppe
    timer0_start();
}

void emission_ir_interrupt_routine(void) {
    // Le Timer 0 a généré cette interruption, on efface le drapeau via l'abstraction si nécessaire
    // L'acquittement se fait directement dans le hardware pour l'instant
    LPC_TIM0->IR = 1; // On garde cet acquittement ici car NVIC gère globalement, mais c'est propre au Timer0

    // Gérer l'état de l'émission
    update_pwm_state();
}

static void update_pwm_state(void) {
    uint8_t frame_counter = get_ir_frame_counter();
    uint8_t seq_index = get_ir_seq_index();
    uint8_t seq_length = get_ir_seq_length();

    // PHASE BLANC LORS DES CYCLES 3, 4 et 5 (Silence entre les rafales)
    if (frame_counter >= 3) {
        pwm_disable_ir_output();

        seq_index++;
        if (seq_index >= seq_length) {
            seq_index = 0;
            frame_counter++;
            if (frame_counter >= 6) {
                frame_counter = 0; // On reprend un nouveau cycle d'émission
                preparer_trame(get_robot_number(), get_robot_vitesse(), (uint8_t)get_robot_status());
            }
        }
        
        set_ir_seq_index(seq_index);
        set_ir_frame_counter(frame_counter);
        return;
    }

    // PHASE D'ÉMISSION LORS DES CYCLES 0, 1, 2
    if (get_ir_sequence_at(seq_index) == 1) {
        // Reset le compteur pour synchroniser la phase à 38kHz
        pwm_reset_counter_ir();
        // Activer la sortie
        pwm_enable_ir_output();
    } else {
        // Blanc : Désactiver la sortie
        pwm_disable_ir_output();
    }

    // Passage au temps 't' suivant
    seq_index++;

    // Baisser la patte de synchro à la fin de l'entête (L'entête fait 2 temps : un "1" et un "0")
    if (seq_index == 2 && frame_counter < 3) {
        LPC_GPIO2->FIOCLR = PIN_SYNC_IR;
    }

    if (seq_index >= seq_length) {
        // Une trame est complétement transmise
        seq_index = 0;
        frame_counter++;
        pwm_disable_ir_output();
    }
    
    set_ir_seq_index(seq_index);
    set_ir_frame_counter(frame_counter);
}

void test_emission_ir(void) {
    // Test de préparation d'une trame IR
    preparer_trame(0x0A, 0x05, 0x03); // id:10, vitesse:5 (soit 25%), status:3 (0011)
}

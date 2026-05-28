#include "emissionIR.h"
#include <stdint.h>

uint8_t ir_sequence[MAX_SEQ_LENGTH];
volatile uint8_t seq_index = 0;
volatile uint8_t seq_length = 0;
volatile uint8_t frame_counter = 0;

uint16_t payload;

// Fonction pour préparer la séquence d'enveloppe IR
void preparer_trame(uint8_t id, uint8_t vitesse, uint8_t status) {
    // Sécurité: sortie anticipée si les valeurs dépassent 4 bits (> 15)
    if (id > 0x0F || vitesse > 0x0F || status > 0x0F) {
        return;
    }

    // Calcul du checksum sur 4 bits (complément à 2)
    uint8_t somme = (id + vitesse + status) & 0x0F;
    uint8_t checksum = ((~somme) + 1) & 0x0F;

    // Payload concaténé sur 16 bits
    payload = (id << 12) | (vitesse << 8) | (status << 4) | checksum;

    uint8_t idx = 0;

    // 1. Bit de Start : 1 impulsions, 1 blanc (10)
    ir_sequence[idx++] = 1;
    ir_sequence[idx++] = 0;

    // 2. Traitement des 16 bits de données du MSB vers le LSB
    for (int i = 15; i >= 0; i--) {
        uint8_t bit = (payload >> i) & 1;
        if (bit == 1) {
            // '1' logique = "110" (2t impulsions, 1t blanc)
            ir_sequence[idx++] = 1;
            ir_sequence[idx++] = 1;
            ir_sequence[idx++] = 0;
        } else {
            // '0' logique = "100" (1t impulsions, 2t blanc)
            ir_sequence[idx++] = 1;
            ir_sequence[idx++] = 0;
            ir_sequence[idx++] = 0;
        }
    }

    // 3. Lancement de la machine d'état (Interrupt Timer)
    seq_length = idx;
    seq_index = 0;

    // On ne démarre le timer que s'il n'était pas déjà en train de tourner
    if (!(LPC_TIM0->TCR & 1)) {
        LPC_TIM0->TCR = (1 << 1); // Reset
        LPC_TIM0->TCR = (1 << 0); // Start
    }
}

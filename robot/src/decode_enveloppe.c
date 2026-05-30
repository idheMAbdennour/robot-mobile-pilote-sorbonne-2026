/**
 * @file decode_enveloppe.c
 * @brief Fichier du module decode_enveloppe.
 */

#include "decode_enveloppe.h"

#include <stdio.h>

#include "robot_state.h"
#include "uart.h"
#include "moteur.h"
#include "capteur_inductif.h"

/* ==========================================================================
 * DÉFINITIONS ET MACROS
 * ========================================================================== */
typedef enum {
    SYMBOL_E = 0,
    SYMBOL_e = 1,
    SYMBOL_0 = 2,
    SYMBOL_o = 3,
    SYMBOL_1 = 4,
    SYMBOL_i = 5,
    SYMBOL_BLANK = 6,
    SYMBOL_INVALID = 7
} Symbol_t;

// Chronogrammes attendus (en microsecondes)
#define PERIOD_E       3250
#define PERIOD_e       2750
#define PERIOD_0       750
#define PERIOD_o       1250
#define PERIOD_1       1750
#define PERIOD_i       2250
#define ERROR_MARGIN_PERCENT 15 // Marge de tolérance ajustée

#define MARGIN_E       ((PERIOD_E * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_e       ((PERIOD_e * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_0       ((PERIOD_0 * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_o       ((PERIOD_o * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_1       ((PERIOD_1 * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_i       ((PERIOD_i * ERROR_MARGIN_PERCENT) / 100)

/* ==========================================================================
 * VARIABLES GLOBALES PRIVÉES (MACHINE D'ÉTAT)
 * ========================================================================== */
typedef enum {
    STATE_WAIT_HEADER,
    STATE_COLLECT_DATA
} DecodeState_t;

typedef struct {
    DecodeState_t state;
    uint8_t bit_count;
    uint32_t data;
} Decoder_t;

static Decoder_t decoder_nord = {STATE_WAIT_HEADER, 0, 0};
static Decoder_t decoder_sud = {STATE_WAIT_HEADER, 0, 0};

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PRIVÉES
 * ========================================================================== */

static uint8_t is_rest_duration_valid(uint16_t rest_us) {
    // Un repos "court" est ~500us. 
    // Un repos "long" (quand on rate les symboles de l'autre fil) peut aller jusqu'à 500us + 3250us + 500us = 4250us.
    // On fixe une fourchette généreuse : 350us à 5000us pour accepter les repos longs de trame alternée.
    return (rest_us >= 350 && rest_us <= 5000);
}

static Symbol_t period_us_to_symbol(uint16_t period_us) {
    if (period_us >= PERIOD_E - MARGIN_E && period_us <= PERIOD_E + MARGIN_E) return SYMBOL_E;
    if (period_us >= PERIOD_e - MARGIN_e && period_us <= PERIOD_e + MARGIN_e) return SYMBOL_e;
    if (period_us >= PERIOD_0 - MARGIN_0 && period_us <= PERIOD_0 + MARGIN_0) return SYMBOL_0;
    if (period_us >= PERIOD_o - MARGIN_o && period_us <= PERIOD_o + MARGIN_o) return SYMBOL_o;
    if (period_us >= PERIOD_1 - MARGIN_1 && period_us <= PERIOD_1 + MARGIN_1) return SYMBOL_1;
    if (period_us >= PERIOD_i - MARGIN_i && period_us <= PERIOD_i + MARGIN_i) return SYMBOL_i;
    return SYMBOL_INVALID;
}

static uint8_t is_nord_symbol(Symbol_t sym) {
    return (sym == SYMBOL_E || sym == SYMBOL_0 || sym == SYMBOL_1);
}

static uint8_t is_sud_symbol(Symbol_t sym) {
    return (sym == SYMBOL_e || sym == SYMBOL_o || sym == SYMBOL_i);
}

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PUBLIQUES
 * ========================================================================== */
void decode_enveloppe_commande(uint16_t period_us, uint16_t rest_duration_us) {
    Symbol_t symbol = period_us_to_symbol(period_us);

    // L'en-tête (E ou e) peut arriver après un long silence inter-trames.
    // On ne vérifie le repos que si ce n'est pas un symbole d'entête.
    if (symbol != SYMBOL_E && symbol != SYMBOL_e) {
        if (!is_rest_duration_valid(rest_duration_us)) {
            decoder_nord.state = STATE_WAIT_HEADER;
            decoder_sud.state = STATE_WAIT_HEADER;
            return;
        }
    }

    if (symbol == SYMBOL_INVALID) {
        decoder_nord.state = STATE_WAIT_HEADER;
        decoder_sud.state = STATE_WAIT_HEADER;
        return;
    }

    uint8_t decoded = 0;
    wire_trame_t final_trame;
    uint8_t my_id = get_robot_number();

    if (is_nord_symbol(symbol)) {
        if (symbol == SYMBOL_E) {
            decoder_nord.state = STATE_COLLECT_DATA;
            decoder_nord.bit_count = 0;
            decoder_nord.data = 0;
        } else if (decoder_nord.state == STATE_COLLECT_DATA) {
            decoder_nord.data = (decoder_nord.data << 1) | (symbol == SYMBOL_1 ? 1 : 0);
            decoder_nord.bit_count++;
            
            if (decoder_nord.bit_count == 14) {
                wire_trame_t trame_nord;
                trame_nord.type = (decoder_nord.data >> 11) & 0x07;
                trame_nord.robot_id = (decoder_nord.data >> 7) & 0x0F;
                trame_nord.parameter = decoder_nord.data & 0x7F;
                
                final_trame = trame_nord;
                decoded = 1;
                decoder_nord.state = STATE_WAIT_HEADER;
            }
        }
    } else if (is_sud_symbol(symbol)) {
        if (symbol == SYMBOL_e) {
            decoder_sud.state = STATE_COLLECT_DATA;
            decoder_sud.bit_count = 0;
            decoder_sud.data = 0;
        } else if (decoder_sud.state == STATE_COLLECT_DATA) {
            decoder_sud.data = (decoder_sud.data << 1) | (symbol == SYMBOL_i ? 1 : 0);
            decoder_sud.bit_count++;
            
            if (decoder_sud.bit_count == 14) {
                wire_trame_t trame_sud;
                trame_sud.type = (decoder_sud.data >> 11) & 0x07;
                trame_sud.robot_id = (decoder_sud.data >> 7) & 0x0F;
                trame_sud.parameter = decoder_sud.data & 0x7F;
                
                // Priorité au Sud si l'ID correspond, sinon on garde la dernière trame (ou Nord)
                if (!decoded || trame_sud.robot_id == my_id) {
                    final_trame = trame_sud;
                    decoded = 1;
                }
                decoder_sud.state = STATE_WAIT_HEADER;
            }
        }
    }

    // Transmission au système principal
    if (decoded) {
        set_wire_trame(&final_trame);
    }
}

void decode_enveloppe_process_command(const wire_trame_t *trame) {
    char debug_buf[64];
    
    if (!trame || trame->robot_id != get_robot_number()) {
        return; // Message invalide ou ne nous est pas destiné
    }

    // Avertir par UART qu'une trame a été validée
    sprintf(debug_buf, "\r\n!!! TRAME RECUE !!! Type:%d Robot:%d Param:%d\r\n", 
            trame->type, trame->robot_id, trame->parameter);
    uart0_send_string(debug_buf);

    switch (trame->type) {
        case 0b000: // Vitesse
            set_robot_vitesse((uint8_t)trame->parameter);
            changer_pwm_moteurs((uint8_t)trame->parameter, (uint8_t)trame->parameter);
            break;
            
        case 0b001: // Direction
            // À implémenter selon la logique de direction
            break;
            
        case 0b011: // Commande de la diode de signalisation
            // À implémenter avec le module de statut
            break;
            
        case 0b110: // Validation du mode Hardware pour le débuggage
            capteur_inductif_receive_wire_command(trame->type);
            moteurs_receive_wire_command(trame->type);
            break;
            
        case 0b111: // Demande d'état du module de débuggage
            capteur_inductif_receive_wire_command(trame->type);
            moteurs_receive_wire_command(trame->type);
            break;
            
        default:
            // Autres commandes (servomoteurs, IR, balises) ignorées pour l'instant
            break;
    }
}

uint8_t decode_enveloppe_get_buffer_index(void) {
    if (decoder_nord.state == STATE_COLLECT_DATA) return decoder_nord.bit_count + 1;
    if (decoder_sud.state == STATE_COLLECT_DATA) return decoder_sud.bit_count + 1;
    return 0;
}

void test_print_buffer(void) {
    char buffer[128];
    int pos = 0;

    // Affichage Nord
    pos += sprintf(buffer + pos, "N[");
    pos += sprintf(buffer + pos, (decoder_nord.state == STATE_COLLECT_DATA) ? "E" : ".");
    for (int i = 0; i < 14; i++) {
        if (i < decoder_nord.bit_count) {
            uint8_t bit = (decoder_nord.data >> (decoder_nord.bit_count - 1 - i)) & 1;
            pos += sprintf(buffer + pos, "%c", bit ? '1' : '0');
        } else {
            pos += sprintf(buffer + pos, ".");
        }
    }
    pos += sprintf(buffer + pos, "]");

    // Affichage Sud
    pos += sprintf(buffer + pos, " S[");
    pos += sprintf(buffer + pos, (decoder_sud.state == STATE_COLLECT_DATA) ? "e" : ".");
    for (int i = 0; i < 14; i++) {
        if (i < decoder_sud.bit_count) {
            uint8_t bit = (decoder_sud.data >> (decoder_sud.bit_count - 1 - i)) & 1;
            pos += sprintf(buffer + pos, "%c", bit ? 'i' : 'o');
        } else {
            pos += sprintf(buffer + pos, ".");
        }
    }
    pos += sprintf(buffer + pos, "]\r\n");

    uart0_send_string(buffer);
}

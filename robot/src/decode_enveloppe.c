/**
 * @file decode_enveloppe.c
 * @brief Fichier du module decode_enveloppe.
 */

#include "decode_enveloppe.h"

#include <stdio.h>

#include "robot_state.h"
#include "uart.h"

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
#define REST_DURATION  500
#define ERROR_MARGIN_PERCENT 15 // Marge de tolérance ajustée

#define MARGIN_E       ((PERIOD_E * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_e       ((PERIOD_e * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_0       ((PERIOD_0 * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_o       ((PERIOD_o * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_1       ((PERIOD_1 * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_i       ((PERIOD_i * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_REST    ((REST_DURATION * ERROR_MARGIN_PERCENT) / 100)

/* ==========================================================================
 * VARIABLES GLOBALES PRIVÉES
 * ========================================================================== */
#define FRAME_LEN 15
static Symbol_t window_nord[FRAME_LEN] = {SYMBOL_BLANK};
static Symbol_t window_sud[FRAME_LEN]  = {SYMBOL_BLANK};

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void init_windows(void) {
    for (int i = 0; i < FRAME_LEN; i++) {
        window_nord[i] = SYMBOL_BLANK;
        window_sud[i] = SYMBOL_BLANK;
    }
}

static uint8_t is_rest_duration_valid(uint16_t rest_us) {
    uint16_t min_rest = REST_DURATION - MARGIN_REST;
    uint16_t max_rest = REST_DURATION + MARGIN_REST;
    return (rest_us >= min_rest && rest_us <= max_rest);
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

static uint8_t symbol_to_bit(Symbol_t sym) {
    if (sym == SYMBOL_1 || sym == SYMBOL_i) return 1;
    return 0; // Défaut pour 0 et o
}

static void decode_frame(const Symbol_t *window, wire_trame_t *trame) {
    uint32_t data = 0;
    // Extraction des 14 bits (l'index 0 est l'en-tête)
    for (int i = 1; i < 15; i++) {
        data = (data << 1) | symbol_to_bit(window[i]);
    }
    
    // Format du cahier des charges : 3 bits Type, 4 bits RobotID, 7 bits Param
    trame->type = (data >> 11) & 0x07;
    trame->robot_id = (data >> 7) & 0x0F;
    trame->parameter = data & 0x7F;
}

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS PUBLIQUES
 * ========================================================================== */
void decode_enveloppe_commande(uint16_t period_us, uint16_t rest_duration_us) {
    if (!is_rest_duration_valid(rest_duration_us)) {
        init_windows();
        return;
    }

    Symbol_t symbol = period_us_to_symbol(period_us);
    if (symbol == SYMBOL_INVALID) {
        init_windows();
        return;
    }

    // Glissement de la fenêtre correspondante
    if (is_nord_symbol(symbol)) {
        for (int i = 0; i < FRAME_LEN - 1; i++) {
            window_nord[i] = window_nord[i + 1];
        }
        window_nord[FRAME_LEN - 1] = symbol;
    } else if (is_sud_symbol(symbol)) {
        for (int i = 0; i < FRAME_LEN - 1; i++) {
            window_sud[i] = window_sud[i + 1];
        }
        window_sud[FRAME_LEN - 1] = symbol;
    }

    // Validation de la fenêtre NORD
    uint8_t nord_valid = 0;
    if (window_nord[0] == SYMBOL_E) {
        nord_valid = 1;
        for (int i = 1; i < FRAME_LEN; i++) {
            if (window_nord[i] != SYMBOL_0 && window_nord[i] != SYMBOL_1) {
                nord_valid = 0;
                break;
            }
        }
    }

    // Validation de la fenêtre SUD
    uint8_t sud_valid = 0;
    if (window_sud[0] == SYMBOL_e) {
        sud_valid = 1;
        for (int i = 1; i < FRAME_LEN; i++) {
            if (window_sud[i] != SYMBOL_o && window_sud[i] != SYMBOL_i) {
                sud_valid = 0;
                break;
            }
        }
    }

    // Extraction et Priorisation
    uint8_t decoded = 0;
    wire_trame_t final_trame;
    uint8_t my_id = get_robot_number();

    if (nord_valid) {
        decode_frame(window_nord, &final_trame);
        decoded = 1;
        // On purge la fenêtre pour ne pas redécoder au prochain cycle
        for (int i = 0; i < FRAME_LEN; i++) window_nord[i] = SYMBOL_BLANK;
    }

    if (sud_valid) {
        wire_trame_t trame_sud;
        decode_frame(window_sud, &trame_sud);
        
        // Si aucune trame n'a été validée, OU si Sud nous est adressé prioritairement
        if (!decoded || trame_sud.robot_id == my_id) {
            final_trame = trame_sud;
            decoded = 1;
        }
        for (int i = 0; i < FRAME_LEN; i++) window_sud[i] = SYMBOL_BLANK;
    }

    // Transmission au système principal
    if (decoded) {
        set_wire_trame(&final_trame, 1);
        set_new_wire_trame(1);
    }
}

uint8_t decode_enveloppe_get_buffer_index(void) {
    // Calcul fictif pour rétrocompatibilité avec la signature existante
    uint8_t count = 0;
    for (int i = 0; i < FRAME_LEN; i++) {
        if (window_nord[i] != SYMBOL_BLANK) count++;
        if (window_sud[i] != SYMBOL_BLANK) count++;
    }
    return count;
}

void decode_enveloppe_debug_print_uart(void) {
    char buffer[128];
    int pos = 0;

    pos += sprintf(buffer + pos, "N[");
    for (int i = 0; i < FRAME_LEN; i++) {
        char c = '.';
        if (window_nord[i] == SYMBOL_E) c = 'E';
        else if (window_nord[i] == SYMBOL_0) c = '0';
        else if (window_nord[i] == SYMBOL_1) c = '1';
        pos += sprintf(buffer + pos, "%c", c);
    }

    pos += sprintf(buffer + pos, "] S[");
    for (int i = 0; i < FRAME_LEN; i++) {
        char c = '.';
        if (window_sud[i] == SYMBOL_e) c = 'e';
        else if (window_sud[i] == SYMBOL_o) c = 'o';
        else if (window_sud[i] == SYMBOL_i) c = 'i';
        pos += sprintf(buffer + pos, "%c", c);
    }

    sprintf(buffer + pos, "]\r\n");
    uart0_send_string(buffer);
}

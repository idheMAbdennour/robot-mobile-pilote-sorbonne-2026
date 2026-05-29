#include "decode_enveloppe.h"
#include "robotState.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

// --- Énumération des symboles ---
typedef enum {
    SYMBOL_E = 0,  // Entête Nord (3.25 ms)
    SYMBOL_e = 1,  // Entête Sud  (2.75 ms)
    SYMBOL_0 = 2,  // Bit 0 Nord  (0.75 ms)
    SYMBOL_o = 3,  // Bit 0 Sud   (1.25 ms)
    SYMBOL_1 = 4,  // Bit 1 Nord  (1.75 ms)
    SYMBOL_i = 5,  // Bit 1 Sud   (2.25 ms)
    SYMBOL_BLANK = 6, // Pause entre symboles (0.5 ms)
    SYMBOL_INVALID = 7  // Symbole invalide
} Symbol_t;

// --- Configuration des durées (en microsecondes) ---
#define PERIOD_E       3250  // Entête Nord (3.25 ms)
#define PERIOD_e       2750  // Entête Sud  (2.75 ms)
#define PERIOD_0       750   // Bit 0 Nord  (0.75 ms)
#define PERIOD_o       1250  // Bit 0 Sud   (1.25 ms)
#define PERIOD_1       1750  // Bit 1 Nord  (1.75 ms)
#define PERIOD_i       2250  // Bit 1 Sud   (2.25 ms)

#define REST_DURATION  500   // Pause                 (0.5 ms)
#define ERROR_MARGIN_PERCENT 10 // Marge d'erreur 10%

// --- Calcul des marges ---
#define MARGIN_E       ((PERIOD_E * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_e       ((PERIOD_e * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_0       ((PERIOD_0 * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_o       ((PERIOD_o * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_1       ((PERIOD_1 * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_i       ((PERIOD_i * ERROR_MARGIN_PERCENT) / 100)
#define MARGIN_REST    ((REST_DURATION * ERROR_MARGIN_PERCENT) / 100)

// --- Buffer de réception ---
#define BUFFER_SIZE 30
static Symbol_t buffer_nord[BUFFER_SIZE];
static Symbol_t buffer_sud[BUFFER_SIZE];
static uint16_t buffer_index = 0;

// --- Fonction : Valider la durée au repos (0.5 ms avec marge) ---
static uint8_t is_rest_duration_valid(uint16_t rest_us)
{
    uint16_t min_rest = REST_DURATION - MARGIN_REST;
    uint16_t max_rest = REST_DURATION + MARGIN_REST;
    return (rest_us >= min_rest && rest_us <= max_rest);
}

// --- Fonction : Convertir période (µs) → symbole ---
static Symbol_t period_us_to_symbol(uint16_t period_us)
{
    // Vérifier Entête Nord (E)
    if (period_us >= PERIOD_E - MARGIN_E && period_us <= PERIOD_E + MARGIN_E) {
        return SYMBOL_E;
    }
    // Vérifier Entête Sud (e)
    if (period_us >= PERIOD_e - MARGIN_e && period_us <= PERIOD_e + MARGIN_e) {
        return SYMBOL_e;
    }
    // Vérifier Bit 0 Nord (0)
    if (period_us >= PERIOD_0 - MARGIN_0 && period_us <= PERIOD_0 + MARGIN_0) {
        return SYMBOL_0;
    }
    // Vérifier Bit 0 Sud (o)
    if (period_us >= PERIOD_o - MARGIN_o && period_us <= PERIOD_o + MARGIN_o) {
        return SYMBOL_o;
    }
    // Vérifier Bit 1 Nord (1)
    if (period_us >= PERIOD_1 - MARGIN_1 && period_us <= PERIOD_1 + MARGIN_1) {
        return SYMBOL_1;
    }
    // Vérifier Bit 1 Sud (i)
    if (period_us >= PERIOD_i - MARGIN_i && period_us <= PERIOD_i + MARGIN_i) {
        return SYMBOL_i;
    }
    // Symbole invalide
    return SYMBOL_INVALID;
}

// --- Fonction : Déterminer si un symbole est majuscule (Nord) ---
static uint8_t is_uppercase_symbol(Symbol_t sym)
{
    return (sym == SYMBOL_E || sym == SYMBOL_0 || sym == SYMBOL_1);
}

static uint8_t is_header_symbol(Symbol_t sym)
{
    return (sym == SYMBOL_E || sym == SYMBOL_e);
}

// --- Fonction : Ajouter un symbole aux buffers ---
static uint8_t add_symbol_to_buffers(Symbol_t symbol)
{
    if (buffer_index >= BUFFER_SIZE) {
        return 0; // Buffer plein
    }

    if (is_uppercase_symbol(symbol)) {
        // C'est un symbole Nord (majuscule)
        buffer_nord[buffer_index] = symbol;
        buffer_sud[buffer_index] = SYMBOL_BLANK;
    } else {
        // C'est un symbole Sud (minuscule)
        buffer_nord[buffer_index] = SYMBOL_BLANK;
        buffer_sud[buffer_index] = symbol;
    }

    buffer_index++;
    return 1;
}

// --- Fonction : Invalider les buffers ---
static void invalidate_buffers(void)
{
    buffer_index = 0;
    memset(buffer_nord, SYMBOL_BLANK, sizeof(buffer_nord));
    memset(buffer_sud, SYMBOL_BLANK, sizeof(buffer_sud));
}

// --- Fonction : Déterminer le type de message (Nord, Sud ou Entrelacé) ---
typedef enum {
    MSG_TYPE_NORD,
    MSG_TYPE_SUD,
    MSG_TYPE_INTER,
    MSG_TYPE_INVALID
} Message_Type_t;

static Message_Type_t detect_message_type(uint16_t current_length)
{
    uint8_t nord_count = 0;
    uint8_t sud_count = 0;

    for (uint16_t i = 0; i < current_length; i++) {
        if (buffer_nord[i] != SYMBOL_BLANK) nord_count++;
        if (buffer_sud[i] != SYMBOL_BLANK) sud_count++;
    }

    // Cas 1 : Nord seul (15 symboles Nord)
    if (current_length == 15 && nord_count == 15 && sud_count == 0) {
        return MSG_TYPE_NORD;
    }
    // Cas 2 : Sud seul (15 symboles Sud)
    if (current_length == 15 && nord_count == 0 && sud_count == 15) {
        return MSG_TYPE_SUD;
    }
    // Cas 3 : Entrelacé (30 symboles: 15 Nord + 15 Sud, alternance)
    if (current_length == 30 && nord_count == 15 && sud_count == 15) {
        return MSG_TYPE_INTER;
    }

    return MSG_TYPE_INVALID;
}

// --- Fonction : Convertir symbole minuscule en majuscule équivalent ---
static Symbol_t lowercase_to_uppercase(Symbol_t sym)
{
    switch (sym) {
        case SYMBOL_e: return SYMBOL_E;
        case SYMBOL_o: return SYMBOL_0;
        case SYMBOL_i: return SYMBOL_1;
        default: return sym;
    }
}

// --- Fonction : Convertir symbole en bit (0 ou 1) ---
static uint8_t symbol_to_bit(Symbol_t sym)
{
    sym = lowercase_to_uppercase(sym); // Normaliser
    if (sym == SYMBOL_0 || sym == SYMBOL_o) return 0;
    if (sym == SYMBOL_1 || sym == SYMBOL_i) return 1;
    return 0; // Défaut
}

// --- Fonction : Extraire 14 bits de données depuis les symboles ---
static uint32_t extract_data_bits(const Symbol_t *symbols)
{
    uint32_t data = 0;

    // Skip le premier symbole (entête), extraire 14 bits suivants
    for (int i = 1; i < 15; i++) {
        uint8_t bit = symbol_to_bit(symbols[i]);
        data = (data << 1) | bit;
    }

    return data;
}

// --- Fonction : Décoder le message complet ---
static uint8_t decode_message(wire_trame_t *trame, uint16_t current_length)
{
    Message_Type_t msg_type = detect_message_type(current_length);
    Symbol_t decoded_symbols[15];

    if (msg_type == MSG_TYPE_INVALID) {
        return 0;
    }

    // Extraire les 15 symboles selon le type de message
    if (msg_type == MSG_TYPE_NORD) {
        // Tous les symboles du buffer nord
        memcpy(decoded_symbols, buffer_nord, sizeof(decoded_symbols));
    } else if (msg_type == MSG_TYPE_SUD) {
        // Tous les symboles du buffer sud, convertis en majuscules
        for (int i = 0; i < 15; i++) {
            decoded_symbols[i] = lowercase_to_uppercase(buffer_sud[i]);
        }
    } else { // MSG_TYPE_INTER
        // Indices pairs du buffer nord (ou sud, ils sont entrelacés)
        for (int i = 0; i < 15; i++) {
            decoded_symbols[i] = buffer_nord[i * 2];
        }
    }

    // Vérifier l'entête (premier symbole doit être E)
    if (decoded_symbols[0] != SYMBOL_E) {
        return 0;
    }

    // Extraire les 14 bits de données (type 3 bits + robot_id 4 bits + param 7 bits)
    uint32_t data = extract_data_bits(decoded_symbols);

    // Type (bits 13-11)
    trame->type = (data >> 11) & 0x07;
    // Robot ID (bits 10-7)
    trame->robot_id = (data >> 7) & 0x0F;
    // Paramètre (bits 6-0)
    trame->parameter = data & 0x7F;

    return 1;
}

// --- FONCTION PRINCIPALE ---
void decode_enveloppe_commande(uint16_t period_us, uint16_t rest_duration_us)
{
    uint16_t current_length;

    // Valider la durée au repos
    if (!is_rest_duration_valid(rest_duration_us)) {
        invalidate_buffers();
        return;
    }

    // Convertir période → symbole
    Symbol_t symbol = period_us_to_symbol(period_us);

    // Vérifier si symbole valide
    if (symbol == SYMBOL_INVALID) {
        invalidate_buffers();
        return;
    }

    if (buffer_index == 0 && !is_header_symbol(symbol)) {
        return;
    }

    if (buffer_index > 0 && is_header_symbol(symbol) && buffer_index < BUFFER_SIZE) {
        invalidate_buffers();
    }

    // Ajouter symbole aux buffers
    if (!add_symbol_to_buffers(symbol)) {
        return; // Buffer plein (ne devrait pas arriver ici)
    }

    current_length = buffer_index;

    // On tente le décodage dès qu'un format complet possible est disponible.
    // 15 symboles suffisent pour les trames Nord/Sud, 30 pour l'entrelacé.
    if (current_length != 15 && current_length != 30) {
        return;
    }

    {
        wire_trame_t decoded_trame;

        // Décoder le message complet
        if (decode_message(&decoded_trame, current_length)) {
            // Copier dans robotState (flags mis à jour dans le bon ordre)
            set_wire_trame(&decoded_trame, 1);  // Copier la trame + valide=1
            set_new_wire_trame(1);              // Puis setter le flag new

            // Après une trame valide, on repart proprement.
            invalidate_buffers();
            return;
        }

        // Si on a atteint 30 symboles sans décoder, on repart sur une nouvelle recherche.
        if (current_length >= BUFFER_SIZE) {
            invalidate_buffers();
        }
    }
}

uint8_t decode_enveloppe_get_buffer_index(void)
{
    return (uint8_t)buffer_index;
}

void decode_enveloppe_debug_print_uart(void)
{
    char buffer[192];
    uint16_t i;
    uint16_t pos = 0;

    pos += (uint16_t)sprintf(buffer + pos, "BUF[%u] N:", (unsigned)buffer_index);
    for (i = 0; i < buffer_index && pos < (sizeof(buffer) - 4); i++) {
        char c = '.';

        if (buffer_nord[i] == SYMBOL_E) c = 'E';
        else if (buffer_nord[i] == SYMBOL_0) c = '0';
        else if (buffer_nord[i] == SYMBOL_1) c = '1';

        pos += (uint16_t)sprintf(buffer + pos, "%c", c);
    }

    pos += (uint16_t)sprintf(buffer + pos, " S:");
    for (i = 0; i < buffer_index && pos < (sizeof(buffer) - 4); i++) {
        char c = '.';

        if (buffer_sud[i] == SYMBOL_e) c = 'e';
        else if (buffer_sud[i] == SYMBOL_o) c = 'o';
        else if (buffer_sud[i] == SYMBOL_i) c = 'i';

        pos += (uint16_t)sprintf(buffer + pos, "%c", c);
    }

    pos += (uint16_t)sprintf(buffer + pos, "\r\n");
    uart0_send_string(buffer);
}

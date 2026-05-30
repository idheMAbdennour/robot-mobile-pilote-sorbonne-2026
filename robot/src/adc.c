/**
 * @file adc.c
 * @brief Fichier du module adc.
 */

#include "adc.h"

/* ==========================================================================
 * VARIABLES EXTERNES
 * ========================================================================== */
// Variables externes exportées par capteur_inductif.c
extern volatile uint32_t volatile_adc_sum1;
extern volatile uint32_t volatile_adc_sum2;
extern volatile uint32_t volatile_adc_sum3;
extern volatile uint16_t volatile_adc_sample_count;

/* ==========================================================================
 * TYPES PRIVÉS ET ÉTATS
 * ========================================================================== */

// États de l'Arbitre ADC
typedef enum {
    STATE_IDLE = 0,
    STATE_IND_AV,
    STATE_IND_AR,
    STATE_IND_HOR,
    STATE_PROX
} adc_state_t;

/* ==========================================================================
 * VARIABLES GLOBALES PRIVÉES
 * ========================================================================== */
static volatile adc_state_t adc_state = STATE_IDLE;

// Contexte Proximètre
static volatile uint16_t prox_samples_remaining = 0;
static volatile uint16_t prox_samples_total = 0;
static volatile uint32_t prox_accumulator = 0;
static volatile uint16_t prox_final_value = 0;
static volatile int prox_is_ready = 0;

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS D'INITIALISATION
 * ========================================================================== */
void adc_init_shared(uint8_t clkdiv) {
    LPC_SC->PCONP |= (1u << 12);
    LPC_ADC->ADCR = (LPC_ADC->ADCR & ~(0xFF00u | 0xFFu))
                  | (1u << 21)
                  | ((clkdiv & 0xFF) << 8);
                  
    // Définir les priorités: EINT3 (Inductif) doit être prioritaire sur ADC (Proximètre)
    NVIC_SetPriority(EINT3_IRQn, 0); // Plus haute priorité
    NVIC_SetPriority(ADC_IRQn, 1);   // Priorité plus faible
    
    // Activation de l'interruption ADC au niveau du NVIC
    NVIC_EnableIRQ(ADC_IRQn);
}

void adc_pin_config(uint8_t channel) {
    switch(channel) {
        case 0: // AD0.0 -> P0.23
            LPC_PINCON->PINSEL1 &= ~(3u << 14);
            LPC_PINCON->PINSEL1 |=  (1u << 14);
            LPC_PINCON->PINMODE1 &= ~(3u << 14);
            LPC_PINCON->PINMODE1 |=  (2u << 14);
            break;
        case 1: // AD0.1 -> P0.24
            LPC_PINCON->PINSEL1 &= ~(3u << 16);
            LPC_PINCON->PINSEL1 |=  (1u << 16);
            LPC_PINCON->PINMODE1 &= ~(3u << 16);
            LPC_PINCON->PINMODE1 |=  (2u << 16);
            break;
        case 2: // AD0.2 -> P0.25
            LPC_PINCON->PINSEL1 &= ~(3u << 18);
            LPC_PINCON->PINSEL1 |=  (1u << 18);
            LPC_PINCON->PINMODE1 &= ~(3u << 18);
            LPC_PINCON->PINMODE1 |=  (2u << 18);
            break;
        case 3: // AD0.3 -> P0.26
            LPC_PINCON->PINSEL1 &= ~(3u << 20);
            LPC_PINCON->PINSEL1 |=  (1u << 20);
            LPC_PINCON->PINMODE1 &= ~(3u << 20);
            LPC_PINCON->PINMODE1 |=  (2u << 20);
            break;
    }
}

/* ==========================================================================
 * INTERFACE PROXIMETRE
 * ========================================================================== */
void adc_request_proximetre_avg(uint16_t num_samples) {
    // Empêcher l'interruption ADC de modifier l'état pendant qu'on prépare
    NVIC_DisableIRQ(ADC_IRQn);
    
    prox_samples_remaining = num_samples;
    prox_samples_total = num_samples;
    prox_accumulator = 0;
    prox_is_ready = 0;
    
    if (adc_state == STATE_IDLE && num_samples > 0) {
        adc_state = STATE_PROX;
        LPC_ADC->ADCR &= ~0xFF;
        LPC_ADC->ADCR |= (1u << PROXIMETRE_ADC_CH);
        LPC_ADC->ADINTEN = (1u << PROXIMETRE_ADC_CH);
        LPC_ADC->ADCR &= ~(7u << 24);
        LPC_ADC->ADCR |= (1u << 24); // START NOW
    }
    
    NVIC_EnableIRQ(ADC_IRQn);
}

int adc_is_proximetre_ready(void) {
    return prox_is_ready;
}

uint16_t adc_get_proximetre_value(void) {
    prox_is_ready = 0;
    return prox_final_value;
}

/* ==========================================================================
 * INTERFACE CAPTEUR INDUCTIF (PRÉEMPTION)
 * ========================================================================== */
void adc_start_inductif_sequence(void) {
    // Appelée depuis EINT3 (Très Haute Priorité)
    // On avorte brutalement tout ce qui était en cours
    LPC_ADC->ADCR &= ~0xFF; 
    
    // On lance la séquence sur la première bobine
    LPC_ADC->ADCR |= (1u << CAPTEUR_IND_ADC_CH_AV);
    LPC_ADC->ADINTEN = (1u << CAPTEUR_IND_ADC_CH_AV) | (1u << CAPTEUR_IND_ADC_CH_AR) | (1u << CAPTEUR_IND_ADC_CH_HOR);
    LPC_ADC->ADCR &= ~(7u << 24);
    LPC_ADC->ADCR |= (1u << 24); // START NOW
    
    // On change l'état de l'arbitre. S'il était en STATE_PROX,
    // prox_samples_remaining est toujours > 0, donc le proximètre 
    // reprendra automatiquement plus tard.
    adc_state = STATE_IND_AV;
}

/* ==========================================================================
 * MACHINE D'ÉTAT PRINCIPALE (ADC_IRQHandler)
 * ========================================================================== */
void ADC_IRQHandler(void) {
    if (adc_state == STATE_IND_AV) {
        // La macro CAPTEUR_IND_ADC_CH_AV vaut 1, on lit ADDR1
        volatile_adc_sum1 += (uint16_t)((LPC_ADC->ADDR1 >> 4) & 0x0FFF);
        
        LPC_ADC->ADCR &= ~0xFF;
        LPC_ADC->ADCR |= (1u << CAPTEUR_IND_ADC_CH_AR);
        LPC_ADC->ADCR |= (1u << 24);
        
        adc_state = STATE_IND_AR;
        return;
    }
    
    if (adc_state == STATE_IND_AR) {
        // La macro CAPTEUR_IND_ADC_CH_AR vaut 2, on lit ADDR2
        volatile_adc_sum2 += (uint16_t)((LPC_ADC->ADDR2 >> 4) & 0x0FFF);
        
        LPC_ADC->ADCR &= ~0xFF;
        LPC_ADC->ADCR |= (1u << CAPTEUR_IND_ADC_CH_HOR);
        LPC_ADC->ADCR |= (1u << 24);
        
        adc_state = STATE_IND_HOR;
        return;
    }
    
    if (adc_state == STATE_IND_HOR) {
        // La macro CAPTEUR_IND_ADC_CH_HOR vaut 3, on lit ADDR3
        volatile_adc_sum3 += (uint16_t)((LPC_ADC->ADDR3 >> 4) & 0x0FFF);
        volatile_adc_sample_count++;
        
        // Séquence inductive terminée. 
        // L'arbitre décide s'il reprend le proximètre.
        LPC_ADC->ADINTEN = 0; 
        
        if (prox_samples_remaining > 0) {
            adc_state = STATE_PROX;
            LPC_ADC->ADCR &= ~0xFF;
            LPC_ADC->ADCR |= (1u << PROXIMETRE_ADC_CH);
            LPC_ADC->ADINTEN = (1u << PROXIMETRE_ADC_CH);
            LPC_ADC->ADCR |= (1u << 24);
        } else {
            adc_state = STATE_IDLE;
        }
        return;
    }
    
    if (adc_state == STATE_PROX) {
        // La macro PROXIMETRE_ADC_CH vaut 0, on lit ADDR0
        prox_accumulator += (uint16_t)((LPC_ADC->ADDR0 >> 4) & 0x0FFF);
        prox_samples_remaining--;
        
        if (prox_samples_remaining > 0) {
            // On relance pour le prochain échantillon
            LPC_ADC->ADCR &= ~0xFF;
            LPC_ADC->ADCR |= (1u << PROXIMETRE_ADC_CH);
            LPC_ADC->ADCR |= (1u << 24);
        } else {
            // Terminé !
            prox_final_value = (uint16_t)(prox_accumulator / prox_samples_total);
            prox_is_ready = 1;
            adc_state = STATE_IDLE;
            LPC_ADC->ADINTEN = 0;
        }
        return;
    }
    
    // Cas de sécurité (ne devrait jamais arriver)
    LPC_ADC->ADINTEN = 0;
}

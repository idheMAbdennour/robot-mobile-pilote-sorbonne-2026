#include "LPC17xx.h"
#include "son.h"
#include "stepper.h"     /* stepper_isr_step() */
#include <stdint.h>

/* ==========================================================================
 * Sortie HP + Timer 1 partage (ton 1 kHz sur MR0, demi-pas stepper sur MR1).
 * TIM1 tourne librement a 1 us/tick ; MR0/MR1 sont reprogrammes a chaque IRQ
 * (meme principe que le servo sur TIM3 dans proximetre.c).
 * ========================================================================== */
#define SPK_PORT        LPC_GPIO2
#define SPK_PIN         (1u << 4)    /* P2.4 -> grille MOSFET -> HP */

#define TONE_HALF_US    500u         /* 500 us -> 1 kHz */
#define STEP_PERIOD_US  667u         /* demi-pas stepper (~x3). Monter a 1000
                                        si le moteur saute des pas. */

static volatile int tone_on = 0;

void son_init(void) {
    /* P2.4 en sortie GPIO, a 0 (silence) */
    LPC_PINCON->PINSEL4 &= ~(3u << 8);
    SPK_PORT->FIODIR |= SPK_PIN;
    SPK_PORT->FIOCLR  = SPK_PIN;

    /* Timer 1 : free running 1 us, IRQ sur MR0 et MR1, SANS reset auto */
    LPC_SC->PCONP    |= (1u << 2);          /* alim TIM1 */
    LPC_SC->PCLKSEL0 &= ~(3u << 4);         /* PCLK_TIM1 = CCLK/4 (~25 MHz) */
    LPC_TIM1->TCR  = 0x02;                   /* reset */
    LPC_TIM1->PR   = 25u - 1u;               /* 1 us / tick */
    LPC_TIM1->MCR  = (1u << 0) | (1u << 3);  /* IRQ sur MR0 (bit0) et MR1 (bit3) */
    LPC_TIM1->IR   = 0x3F;
    LPC_TIM1->TCR  = 0x01;                    /* start */
    LPC_TIM1->MR0  = LPC_TIM1->TC + TONE_HALF_US;
    LPC_TIM1->MR1  = LPC_TIM1->TC + STEP_PERIOD_US;
    NVIC_EnableIRQ(TIMER1_IRQn);
}

void son_tone_start(void) { tone_on = 1; }
void son_tone_stop(void)  { tone_on = 0; SPK_PORT->FIOCLR = SPK_PIN; }
int  son_tone_is_on(void) { return tone_on; }

// void TIMER1_IRQHandler(void) {
//     uint32_t ir = LPC_TIM1->IR;

//     if (ir & (1u << 0)) {                    /* --- MR0 : ton 1 kHz --- */
//         LPC_TIM1->IR = (1u << 0);
//         if (tone_on) {
//             if (SPK_PORT->FIOPIN & SPK_PIN) SPK_PORT->FIOCLR = SPK_PIN;
//             else                            SPK_PORT->FIOSET = SPK_PIN;
//         }
//         LPC_TIM1->MR0 = LPC_TIM1->TC + TONE_HALF_US;
//     }

//     if (ir & (1u << 1)) {                    /* --- MR1 : un demi-pas stepper --- */
//         LPC_TIM1->IR = (1u << 1);
//         stepper_isr_step();                  /* ne fait rien si aucun pas en attente */
//         LPC_TIM1->MR1 = LPC_TIM1->TC + STEP_PERIOD_US;
//     }
// }

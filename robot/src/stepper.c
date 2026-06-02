#include "LPC17xx.h"
#include "stepper.h"
#include <stdint.h>

/* ==========================================================================
 * BROCHES : P1.0/1.1/1.4/1.8 -> ULN2003 IN1..IN4 (libres dans le projet)
 * ========================================================================== */
#define STEP_IN1    (1u << 0)   /* P1.0 */
#define STEP_IN2    (1u << 1)   /* P1.1 */
#define STEP_IN3    (1u << 4)   /* P1.4 */
#define STEP_IN4    (1u << 8)   /* P1.8 */
#define STEP_MASK   (STEP_IN1 | STEP_IN2 | STEP_IN3 | STEP_IN4)

#define STEPS_PER_REV   8192    /* demi-pas / tour (a verifier : si 2 tours -> 4096) */
#define NUM_POSITIONS   5

/* Cadence des pas = MR1 du Timer 1, configuree dans son.c (STEP_PERIOD_US). */

static const int POSITION_STEPS[NUM_POSITIONS] = {
    0,
    (STEPS_PER_REV * 1) / NUM_POSITIONS,
    (STEPS_PER_REV * 2) / NUM_POSITIONS,
    (STEPS_PER_REV * 3) / NUM_POSITIONS,
    (STEPS_PER_REV * 4) / NUM_POSITIONS
};

static const uint8_t HALF_STEP[8][4] = {
    {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
    {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};

/* Etat partage avec l'ISR TIM1 (son.c) */
static volatile int current_step_idx = 0;
static volatile int current_abs_step = 0;
static volatile int current_position = 0;
static volatile int pending_position = 0;
static volatile int steps_remaining  = 0;
static volatile int step_dir         = 0;

static void step_apply(void) {
    LPC_GPIO1->FIOCLR = STEP_MASK;
    if (HALF_STEP[current_step_idx][0]) LPC_GPIO1->FIOSET = STEP_IN1;
    if (HALF_STEP[current_step_idx][1]) LPC_GPIO1->FIOSET = STEP_IN2;
    if (HALF_STEP[current_step_idx][2]) LPC_GPIO1->FIOSET = STEP_IN3;
    if (HALF_STEP[current_step_idx][3]) LPC_GPIO1->FIOSET = STEP_IN4;
}

static void stepper_off(void) {
    LPC_GPIO1->FIOCLR = STEP_MASK;   /* bobines coupees */
}

void stepper_init(void) {
    LPC_PINCON->PINSEL2 &= ~(3u << 0);    /* P1.0 GPIO */
    LPC_PINCON->PINSEL2 &= ~(3u << 2);    /* P1.1 GPIO */
    LPC_PINCON->PINSEL2 &= ~(3u << 8);    /* P1.4 GPIO */
    LPC_PINCON->PINSEL2 &= ~(3u << 16);   /* P1.8 GPIO */
    LPC_GPIO1->FIODIR |= STEP_MASK;
    stepper_off();

    current_position = 0;
    current_abs_step = 0;
    current_step_idx = 0;
    pending_position = 0;
    steps_remaining  = 0;
    step_dir         = 0;
    /* Pas de timer ici : c'est son_init() qui demarre TIM1. */
}

void stepper_set_zero_manually(void) {
    NVIC_DisableIRQ(TIMER1_IRQn);
    current_position = 0;
    current_abs_step = 0;
    current_step_idx = 0;
    steps_remaining  = 0;
    NVIC_EnableIRQ(TIMER1_IRQn);
    stepper_off();
}

void stepper_home(void) { stepper_set_zero_manually(); }

int stepper_is_busy(void) { return (steps_remaining != 0); }

void stepper_move_to(DrumPosition target) {
    int target_position = (int)target;
    int target_abs, delta;

    if (target_position < 0 || target_position >= NUM_POSITIONS) return;

    NVIC_DisableIRQ(TIMER1_IRQn);   /* section critique : etat lu par l'ISR */

    if (target_position == current_position && steps_remaining == 0) {
        NVIC_EnableIRQ(TIMER1_IRQn);
        return;
    }

    target_abs = POSITION_STEPS[target_position];
    delta = target_abs - current_abs_step;
    if (delta >  STEPS_PER_REV / 2) delta -= STEPS_PER_REV;
    if (delta < -STEPS_PER_REV / 2) delta += STEPS_PER_REV;

    if (delta == 0) {
        current_position = target_position;
        steps_remaining  = 0;
        NVIC_EnableIRQ(TIMER1_IRQn);
        stepper_off();
        return;
    }

    step_dir         = (delta > 0) ? +1 : -1;
    steps_remaining  = (delta > 0) ? delta : -delta;
    pending_position = target_position;

    NVIC_EnableIRQ(TIMER1_IRQn);
    /* Le mouvement se fait via stepper_isr_step(), appele par l'ISR TIM1. */
}

void stepper_show_letter(char c) {
    switch (c) {
        case ' ': stepper_move_to(POSITION_0); break;
        case 'A': stepper_move_to(POSITION_1); break;
        case 'B': stepper_move_to(POSITION_2); break;
        case 'C': stepper_move_to(POSITION_3); break;
        case 'D': stepper_move_to(POSITION_4); break;
        default:  break;
    }
}

/* Appele depuis l'ISR de TIM1 (son.c), une fois par periode de pas. */
void stepper_isr_step(void) {
    if (steps_remaining <= 0) return;

    if (step_dir > 0) current_step_idx = (current_step_idx + 1) & 7;
    else              current_step_idx = (current_step_idx + 7) & 7;

    step_apply();

    current_abs_step += step_dir;
    if (current_abs_step >= STEPS_PER_REV) current_abs_step -= STEPS_PER_REV;
    if (current_abs_step < 0)              current_abs_step += STEPS_PER_REV;

    steps_remaining--;
    if (steps_remaining == 0) {
        current_position = pending_position;
        stepper_off();
    }
}

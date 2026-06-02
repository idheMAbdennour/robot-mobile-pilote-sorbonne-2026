#ifndef STEPPER_H
#define STEPPER_H

/*
 * Afficheur "lettre de service" (CdC p.11) : tambour 5 positions
 * (vide + colisA/B/C/D), 28BYJ-48 + ULN2003, demi-pas.
 *
 * Pins P1.0/1.1/1.4/1.8 -> ULN2003 IN1..IN4 (libres dans le projet).
 *
 * NON BLOQUANT : les demi-pas sont cadences par Timer 1, dont le proprietaire
 * est maintenant son.c (TIM1 partage : ton 1 kHz + pas). stepper_isr_step()
 * est appele depuis l'ISR de TIM1 dans son.c. Donc dans main :
 *   stepper_init();   // GPIO du moteur
 *   son_init();       // demarre TIM1 (ton + cadence des pas)
 */

typedef enum {
    POSITION_0 = 0,   /* vide    */
    POSITION_1 = 1,   /* colis A */
    POSITION_2 = 2,   /* colis B */
    POSITION_3 = 3,   /* colis C */
    POSITION_4 = 4    /* colis D */
} DrumPosition;

void stepper_init(void);              /* GPIO seulement (le timer = son_init) */
void stepper_set_zero_manually(void); /* declare la position courante = vide  */
void stepper_home(void);              /* compat : = stepper_set_zero_manually */
void stepper_move_to(DrumPosition target);
void stepper_show_letter(char c);     /* ' ' / 'A' / 'B' / 'C' / 'D' */
int  stepper_is_busy(void);

/* Avance d'un demi-pas si un mouvement est en attente. Appele par l'ISR TIM1. */
void stepper_isr_step(void);

#endif

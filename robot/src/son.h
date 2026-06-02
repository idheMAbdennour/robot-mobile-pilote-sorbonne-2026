#ifndef SON_H
#define SON_H

/*
 * Avertisseur sonore 1 kHz (CdC p.10) sur haut-parleur via MOSFET (P2.4).
 *
 * Le HP n'a pas d'oscillateur interne : le 1 kHz est genere par le MC en
 * basculant la patte toutes les 500 us (Timer 1).
 *
 * /!\ NE JAMAIS brancher le HP (~8 ohms) directement sur la patte du LPC :
 *     courant ~0.4 A -> destruction du port. Passer par un MOSFET (cf. cablage).
 *
 * Timer 1 est partage : MR0 = ton 1 kHz, MR1 = cadence des demi-pas du stepper
 * (seul timer libre du projet). son_init() demarre TIM1 ; il doit donc etre
 * appele APRES stepper_init().
 */

void son_init(void);        /* config P2.4 + Timer 1, demarre TIM1 */
void son_tone_start(void);  /* lance le 1 kHz */
void son_tone_stop(void);   /* coupe le son */
int  son_tone_is_on(void);

#endif

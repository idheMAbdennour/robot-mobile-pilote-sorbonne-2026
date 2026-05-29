#ifndef PROXIMETRE_H
#define PROXIMETRE_H
 
#include <stdint.h>
 
#define PIN_PROX_SW1 (1 << 12) // P0.12
#define PIN_PROX_SW2 (1 << 13) // P0.13
/* Nombre max. de mesures pour UN seul passage de balayage.
 * Mode le plus large = ±60° avec un pas de 5°  ->  (2*60)/5 + 1 = 25.
 * Les modes étroits en remplissent moins — le nombre réel est obtenu via get_proxi_count(). */
#define NUM_PROXI_MEASUREMENTS  25
 
/* ---- API du module ---- */
void init_proximetre(void);                  /* servo(Timer3) + ADC + buzzer + interrupteurs */
void proximetre_interrupt_routine(void);     /* appeler depuis EINT3_IRQHandler (interrupteurs) */
void proximetre_timer_interrupt_routine(void); /* appeler depuis TIMER3_IRQHandler (servo) */
void proximetre_run_balayage(void);          /* un passage ; alterne avant/arrière */
void get_proxi_distances(int32_t *out);      /* copie le dernier passage (cm) */
int  get_proxi_count(void);                  /* nombre de points réellement enregistrés */
void debug_proximetre_send_frame(void);      /* envoie une trame "T ... \r\n" via UART0 */
 
#endif /* PROXIMETRE_H */
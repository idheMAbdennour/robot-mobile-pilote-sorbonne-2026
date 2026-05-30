/**
 * @file proximetre.h
 * @brief Fichier du module proximetre.
 */

#ifndef PROXIMETRE_H
#define PROXIMETRE_H

#include <stdint.h>

/* ==========================================================================
 * DÉFINITIONS (PINS ET CONSTANTES)
 * ========================================================================== */
#define PIN_PROX_SW1            (1u << 30)  // P1.30 - Mode Proximetre bit 0
#define PIN_PROX_SW2            (1u << 31)  // P1.31 - Mode Proximetre bit 1
#define SERVO_PIN               (1u << 3)   // P2.3  - Signal Servo
#define BUZZ_PIN                (1u << 4)   // P2.4  - Buzzer

#define PORT_PROX_SW            LPC_GPIO1
#define PORT_SERVO              LPC_GPIO2
#define PORT_BUZZ               LPC_GPIO2

// L'angle maximal de balayage (ex: 60 pour ±60°)
#define PROXI_MAX_ANGLE_DEG     60

// Le pas angulaire entre deux mesures
#define PROXI_STEP_DEG          5

// Taille du tableau des mesures calculée dynamiquement
#define NUM_PROXI_MEASUREMENTS  ((2 * PROXI_MAX_ANGLE_DEG) / PROXI_STEP_DEG + 1)

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS
 * ========================================================================== */

/**
 * @brief Initialise le proximètre (Timer3 pour Servo, ADC, Buzzer, GPIO).
 */
void init_proximetre(void);

/**
 * @brief Routine d'interruption du Timer3 pour la génération du signal Servo logiciel.
 */
void proximetre_timer_interrupt_routine(void);

/**
 * @brief Exécute un cycle de balayage complet du proximètre.
 */
void proximetre_run_balayage(void);

/**
 * @brief Récupère le buffer local des mesures de distance.
 * @param out Pointeur vers le tableau de destination.
 */
void get_local_proxi_buffer(int32_t *out);

/**
 * @brief Récupère le nombre de mesures effectuées.
 * @return Le nombre de mesures.
 */
int get_proxi_count(void);

/**
 * @brief Envoie l'état du proximètre via UART (Debug).
 */
void debug_proximetre_send_frame(void);

/**
 * @brief Fonction de test du proximètre, décommentable dans le main.
 */
void test_proximetre_module(void);

#endif // PROXIMETRE_H

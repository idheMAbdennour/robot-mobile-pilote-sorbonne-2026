/**
 * @file main.c
 * @brief Fichier du module main.
 */

#include <stdint.h>
#include <stdio.h>

#include "LPC17xx.h"

#include "adc.h"
#include "buttons.h"
#include "capteur_inductif.h"
#include "decode_enveloppe.h"
#include "dtmf.h"
#include "emission_ir.h"
#include "moteur.h"
#include "proximetre.h"
#include "recep_spi.h"
#include "robot_state.h"
#include "status.h"
#include "uart.h"
#include "asservissement.h"

/**
 * @file main.c
 * @brief Programme de test pour la réception SPI et le filtrage de l'odométrie.
 */

#include "LPC17xx.h"
#include "recep_spi.h"

// Récupération des variables globales du module SPI
extern volatile uint32_t g_Vg;
extern volatile uint32_t g_Vd;
extern volatile uint32_t g_Pg;
extern volatile uint32_t g_Pd;

// Variables pour le stockage des valeurs filtrées
uint32_t g_Vg_filtree = 0;
uint32_t g_Vd_filtree = 0;

/* ==========================================================================
 * FONCTION DE FILTRAGE (Moyenne glissante sur 3 échantillons)
 * ========================================================================== */
/**
 * @brief Applique un filtre moyenneur sur les 3 dernières mesures de vitesse.
 * @param nouvelle_valeur La dernière mesure brute lue sur le SPI.
 * @param historique Tableau de 3 éléments stockant l'historique.
 * @return La valeur moyenne calculée.
 */
uint32_t filtrer_vitesse(uint32_t nouvelle_valeur, uint32_t* historique) {
    // On décale l'historique (les anciennes valeurs laissent la place à la nouvelle)
    historique[0] = historique[1];
    historique[1] = historique[2];
    historique[2] = nouvelle_valeur;

    // Calcul de la moyenne brute
    return (historique[0] + historique[1] + historique[2]) / 3;
}

/* ==========================================================================
 * PROGRAMME PRINCIPAL
 * ========================================================================== */
int main(void) {
    // 1. Initialisation du système (Horloges du LPC1769 à 120 MHz)
    SystemInit();

    // 2. Initialisation des historiques pour le filtre des deux roues
    uint32_t hist_Vg[3] = {0, 0, 0};
    uint32_t hist_Vd[3] = {0, 0, 0};

    // 3. Initialisation du module SPI (et de l'interruption EINT1 à 50 Hz)
    init_recep_spi();

    /* Variables locales pour détecter le changement de valeur 
       (Évite de recalculer la moyenne pour rien en boucle) */
    uint32_t ancienne_Vg = 0;
    uint32_t ancienne_Vd = 0;

    // 4. Boucle infinie
    while (1) {
					
        // Si le FPGA a envoyé une nouvelle trame (détecté par le changement de valeur brute)
        if (g_Vg != ancienne_Vg || g_Vd != ancienne_Vd) {
            
            // Mise à jour des témoins
            ancienne_Vg = g_Vg;
            ancienne_Vd = g_Vd;

            // Application du filtre moyenneur de 3 échantillons pour l'odométrie
            g_Vg_filtree = filtrer_vitesse(g_Vg, hist_Vg);
            g_Vd_filtree = filtrer_vitesse(g_Vd, hist_Vd);

            /* --- Zone d'action de ton Robot ---
             * C'est ici, 50 fois par seconde, juste après le filtrage,
             * que tu vas insérer tes fonctions de calcul de PID / PWM
             * pour l'asservissement des moteurs.
             * * Exemple : 
             * corriger_asservissement(g_Vg_filtree, g_Vd_filtree, g_Pg, g_Pd);
             */
        }

        // Le reste du temps, le CPU peut traiter d'autres tâches (Supervision, UART, etc.)
        __WFI(); // Optionnel : Met le CPU en veille légère en attendant la prochaine interruption
    }
}
/*
int main(void)
{
		// Initialisation du système
		SystemInit();

		// Initialisation partagée de l'ADC (CLKDIV = 4)
		adc_init_shared(ADC_CLKDIV);

		// Initialisations de chaque module selon la nouvelle norme (snake_case)
		init_uart0();
		init_pwm_ir();
		init_timer_enveloppe(250);
		init_moteur_pwm();
		init_moteurs_debug();
		init_proximetre();
		init_capteur_inductif();
		init_robot_id_switches();
		init_buttons();
		init_status_led();
		init_dtmf();
		init_recep_spi();

		// Configuration du SysTick à 50 Hz
		SysTick_Config(SystemCoreClock / 50);

		while (1)
		{
				// Attente du tick de 50Hz géré par SysTick_Handler (dans interruptions.c)
				if (!get_flag_50hz())
				{
						continue;
				}

				// Acquittement du flag
				set_flag_50hz(0);

				// Dépilement de la FIFO des événements du capteur inductif (enveloppe et ADC)
				capteur_inductif_update();

				// Lecture de l'ID du robot via les switchs
				update_robot_id_from_hardware();

				// Mise à jour de la LED RGB depuis l'état centralisé
				set_status_led(get_robot_status());

				// Traitement de la machine d'état DTMF
				dtmf_service();

				// Polling pour la lecture des capteurs via SPI
				static uint8_t spi_cs_index = 0;
				set_spi_cs_val(spi_cs_index);
				spi_cs_index = (spi_cs_index + 1) % 4;

				// -----------------------------------------------------
				// Traitement de l'enveloppe (réception série par fil)
				// -----------------------------------------------------
				wire_trame_t trame;
				if (get_wire_trame(&trame)) {
						decode_enveloppe_process_command(&trame);
				}

				// -----------------------------------------------------
				// Boucle d'asservissement (calcul PID / consigne moteurs)
				// -----------------------------------------------------
				asservissement_update();

				// Balayage du proximètre (équivalent à proximetre_tick / proximetre_run_balayage)
				// Doit tourner même si le debug UART est désactivé.
				proximetre_run_balayage();

				// Envois debug par module
				debug_moteurs_send_frame(); // mode sur P0.4 et P0.5
				debug_inductif_send_frame(); // mode sur P0.0, P0.1 et P0.6
				// test_print_buffer(); // Pour test uniquement, affiche les buffer

				// Emission de la trame du proximètre "T/t ddd..."
				debug_proximetre_send_frame();
		}
}
*/
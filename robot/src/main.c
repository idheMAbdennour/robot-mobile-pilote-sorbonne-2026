/**
 * @file main.c
 * @brief Fichier du module main (Version restaurée et fusionnée).
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
#include "ultrason_recep.h"
#include "asservissement.h"
#include "led_register.h"

/* ==========================================================================
 * PROGRAMME PRINCIPAL ORIGINAL (RESTAURÉ)
 * ========================================================================== */
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
    init_ultrason_recep(); // Ajouté par la fusion (e7f9603)
    init_robot_id_switches();
    init_buttons();
    init_status_led();
    init_dtmf();
    init_recep_spi();

    // Configuration du SysTick à 50 Hz
    SysTick_Config(SystemCoreClock / 50);

    while (1)
    {
        // ====================================================================
        // TÂCHES "TEMPS RÉEL" (Boucle rapide Asynchrone / Polling)
        // ====================================================================
        
        // Dépilement de la FIFO des événements du capteur inductif (enveloppe et ADC)
        capteur_inductif_update();
        
        // Traitement de la machine d'état DTMF
        dtmf_service();

        // Traitement de l'enveloppe (réception série par fil)
        wire_trame_t trame;
        if (get_wire_trame(&trame)) {
            decode_enveloppe_process_command(&trame);
        }

        // ====================================================================
        // TÂCHES PÉRIODIQUES (50 Hz)
        // ====================================================================
        // Attente du tick de 50Hz géré par SysTick_Handler (dans interruptions.c)
        if (!get_flag_50hz())
        {
            continue;
        }

        // Acquittement du flag
        set_flag_50hz(0);

        // Lecture de l'ID du robot via les switchs
        update_robot_id_from_hardware();

        // Mise à jour de la LED RGB depuis l'état centralisé
        set_status_led(get_robot_status());

        // -----------------------------------------------------
        // Calcul de l'Odométrie (Conversion SPI -> Mètres)
        // -----------------------------------------------------
        // Les variables g_Vg et g_Vd sont mises à jour par l'interruption EINT1 (Top 50Hz du FPGA)
        extern volatile uint32_t g_Vg; 
        extern volatile uint32_t g_Vd;

        WheelDelta delta;
        // Le FPGA fournit directement le nombre de ticks effectués sur la période de 20ms (1/50s)
        delta.dd_g = (float)((int32_t)g_Vg) * ODO_TICKS_TO_METER;
        delta.dd_d = (float)((int32_t)g_Vd) * ODO_TICKS_TO_METER;
        delta.dt   = 0.02f; // Période fixe de 50Hz (1/50s)

        // Transmission au robot_state pour utilisation par l'asservissement
        set_wheel_delta(&delta);

        // -----------------------------------------------------
        // Boucle d'asservissement (calcul PID / consigne moteurs)
        // -----------------------------------------------------
        asservissement_update();

        // Balayage du proximètre (équivalent à proximetre_tick / proximetre_run_balayage)
        // Doit tourner même si le debug UART est désactivé.
        proximetre_run_balayage();

        // Envois debug par module
        debug_moteurs_send_frame();  // mode sur P0.4 et P0.5
        debug_inductif_send_frame(); // mode sur P0.0, P0.1 et P0.6
        // test_print_buffer();      // Pour test uniquement, affiche les buffer

        // Emission de la trame du proximètre "T/t ddd..."
        debug_proximetre_send_frame();

        // --- Ultrason : identification du poste --- (Ajouté par la fusion e7f9603)
        ultrason_recep_tick();
        uint8_t poste; 
        char cote;
        if (ultrason_recep_lire(&poste, &cote)) {
            char us_buf[48];
            sprintf(us_buf, "US: poste=%u cote=%c\r\n", poste, cote);
            uart0_send_string(us_buf);
        }   
    }
}


/* ==========================================================================
 * ANCIEN CODE DE TEST DE RÉCEPTION SPI (MIS EN COMMENTAIRE)
 * ========================================================================== 
 
// Récupération des variables globales du module SPI
extern volatile uint32_t g_Vg;
extern volatile uint32_t g_Vd;
extern volatile uint32_t g_Pg;
extern volatile uint32_t g_Pd;

// Variables pour le stockage des valeurs filtrées
uint32_t g_Vg_filtree = 0;
uint32_t g_Vd_filtree = 0;

// Fonction de filtrage (Moyenne glissante sur 3 échantillons)
uint32_t filtrer_vitesse(uint32_t nouvelle_valeur, uint32_t* historique) {
    historique[0] = historique[1];
    historique[1] = historique[2];
    historique[2] = nouvelle_valeur;
    return (historique[0] + historique[1] + historique[2]) / 3;
}

int main_test_spi(void) {
    SystemInit();
    uint32_t hist_Vg[3] = {0, 0, 0};
    uint32_t hist_Vd[3] = {0, 0, 0};
    init_recep_spi();

    uint32_t ancienne_Vg = 0;
    uint32_t ancienne_Vd = 0;

    while (1) {
        if (g_Vg != ancienne_Vg || g_Vd != ancienne_Vd) {
            ancienne_Vg = g_Vg;
            ancienne_Vd = g_Vd;

            g_Vg_filtree = filtrer_vitesse(g_Vg, hist_Vg);
            g_Vd_filtree = filtrer_vitesse(g_Vd, hist_Vd);
        }
        __WFI(); 
    }
}
*/
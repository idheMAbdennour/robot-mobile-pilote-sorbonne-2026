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
            if (get_new_wire_trame()) {
                set_new_wire_trame(0);
                wire_trame_t trame;
                get_wire_trame(&trame, NULL);

                // Si la trame correspond au numéro du robot
                if (trame.robot_id == get_robot_number()) {
                    moteurs_receive_wire_command(trame.type);
                    capteur_inductif_receive_wire_command(trame.type);
                }
            }

            // Balayage du proximètre (équivalent à proximetre_tick / proximetre_run_balayage)
            // Doit tourner même si le debug UART est désactivé.
            proximetre_run_balayage();

            // Envois debug par module
            debug_moteurs_send_frame(); // mode sur P0.4 et P0.5
            debug_inductif_send_frame(); // mode sur P0.0, P0.1 et P0.6
            decode_enveloppe_debug_print_uart(); // Pour test uniquement, affiche les buffer

            // Emission de la trame du proximètre "T/t ddd..."
            debug_proximetre_send_frame();
        }
    }

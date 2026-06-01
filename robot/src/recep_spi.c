/**
 * @file recep_spi.c
 * @brief Implémentation du module de réception SPI par bit-banging pour LPC1769.
 */

#include "recep_spi.h"
#include "LPC17xx.h"

// Variables globales pour stocker les mesures
volatile uint32_t g_Vg = 0;
volatile uint32_t g_Vd = 0;
volatile uint32_t g_Pg = 0;
volatile uint32_t g_Pd = 0;

/* ==========================================================================
 * FONCTIONS DE BAS NIVEAU (CHRONOGRAMMES MANUELS)
 * ========================================================================== */

/**
 * @brief Petite temporisation logicielle pour stabiliser les signaux.
 */
static void spi_delay(void) {
		// Environ 200kHz
    for (volatile int i = 0; i < 75; i++);
}

/**
 * @brief Génère une impulsion d'horloge (Front montant puis front descendant).
 * Le FPGA met à jour son bit sur le front descendant (falling edge).
 */
void TOP_CLK(void) {
    LPC_GPIO0->FIOSET = PIN_SPI_CLK; // Passage à '1'
    spi_delay();
    LPC_GPIO0->FIOCLR = PIN_SPI_CLK; // Passage à '0' -> Le FPGA change de bit ici
    spi_delay();
}

/**
 * @brief Lit l'état de la ligne MISO.
 * @return 1 si la ligne est haute, 0 si elle est basse.
 */
uint8_t LIRE_bit(void) {
    if (LPC_GPIO0->FIOPIN & PIN_SPI_MISO) {
        return 1;
    }
    return 0;
}

/* ==========================================================================
 * FONCTIONS PRINCIPALES
 * ========================================================================== */

void init_recep_spi(void) {
    // Configuration des directions (Sorties pour CLK et CS, Entrée pour MISO)
    LPC_GPIO0->FIODIR |= (PIN_SPI_CLK | PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4);
    LPC_GPIO0->FIODIR &= ~PIN_SPI_MISO;

    // États initiaux (CS désactivés à '1', CLK au repos à '0')
    LPC_GPIO0->FIOSET = (PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4);
    LPC_GPIO0->FIOCLR = PIN_SPI_CLK;

    // Configuration P2.11 pour l'interruption externe EINT1 (Top50 du FPGA)
    LPC_PINCON->PINSEL4 &= ~(3 << 22);
    LPC_PINCON->PINSEL4 |=  (1 << 22);
    LPC_SC->EXTMODE    |= (1 << 1);  // Front
    LPC_SC->EXTPOLAR   |= (1 << 1);  // Montant
    LPC_SC->EXTINT      = (1 << 1);  // Clear flag

    NVIC_SetPriority(EINT1_IRQn, 2);
    NVIC_EnableIRQ(EINT1_IRQn);
}

void set_spi(int cs_num) {
    uint32_t current_cs_pin = 0;
    uint32_t raw_data = 0;

    switch (cs_num) {
        case 0: current_cs_pin = PIN_SPI_CS1; break; // Vg
        case 1: current_cs_pin = PIN_SPI_CS2; break; // Vd
        case 2: current_cs_pin = PIN_SPI_CS3; break; // Pg
        case 3: current_cs_pin = PIN_SPI_CS4; break; // Pd
        default: return;
    }

    // Assurer l'état initial
    LPC_GPIO0->FIOCLR = PIN_SPI_CLK;
    spi_delay();

    // Activation du CS (Niveau bas) -> Le FPGA se prépare et présente le premier bit (b0)
    LPC_GPIO0->FIOCLR = current_cs_pin;
    spi_delay(); 

    // Lecture des 32 bits (LSB en premier)
    for (int i = 0; i < 32; i++) {
        // 1. On lit le bit disponible
        if (LIRE_bit()) {
            raw_data |= (1U << i);
        }
        // 2. On envoie le coup d'horloge pour forcer le FPGA à passer au bit suivant
        TOP_CLK();
    }

    // Désactivation du CS (Niveau haut)
    LPC_GPIO0->FIOSET = current_cs_pin;
    spi_delay();

    // Tri des données reçues
    switch (cs_num) {
        case 0: g_Vg = raw_data & 0x0FFFFFFF; break;
        case 1: g_Vd = raw_data & 0x0FFFFFFF; break;
        case 2: g_Pg = raw_data & 0x000000FF; break;
        case 3: g_Pd = raw_data & 0x000000FF; break;
    }
}

void recep_spi_interrupt_routine(void) {
    set_spi(0);
    set_spi(1);
    set_spi(2);
    set_spi(3);
}
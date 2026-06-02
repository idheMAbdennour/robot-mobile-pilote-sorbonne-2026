/**
 * @file recep_spi.c
 * @brief Implémentation du module odométrie avec filtrage et conversion pour LPC1769
 */

#include "recep_spi.h"
#include "LPC17xx.h"

/* ==========================================================================
 * CONSTANTES PHYSIQUES & PARAMÈTRES
 * ========================================================================== */
#define PI                  3.14159265358979323846f
#define WHEEL_DIAMETER      0.096f       // 100mm = 0.10 mètres
#define ENCODER_RESOLUTION  360.0f      // 360 pas par tour (arbre de sortie)
#define FPGA_WINDOW_SEC     0.16666666f // Fenêtre de calcul vitesse FPGA (166.66 ms)

#define WHEEL_PERIMETER     (PI * WHEEL_DIAMETER) 
#define DISTANCE_PER_STEP   (WHEEL_PERIMETER / ENCODER_RESOLUTION)

/* ==========================================================================
 * VARIABLES INTERNES (ENCAPSULÉES)
 * ========================================================================== */
// Variables brutes issues du SPI
static volatile uint32_t g_Vg = 0;
static volatile uint32_t g_Vd = 0;
static volatile uint32_t g_Pg = 0;
static volatile uint32_t g_Pd = 0;

// Historiques pour filtre moyenneur glissant (3 échantillons demandés)
static uint32_t hist_vg[3] = {0, 0, 0};
static uint32_t hist_vd[3] = {0, 0, 0};

// Structure partagée (Mise à jour sous interruption, lue par le Main)
static volatile odometrie_data_t odo_shared_metrics = {0};

/* ==========================================================================
 * FONCTIONS PRIVÉES / OUTILS BAS NIVEAU
 * ========================================================================== */

static void spi_delay(void) {
    for (volatile int i = 0; i < 75; i++); // Temporisation logicielle (~200kHz)
}

static void TOP_CLK(void) {
    LPC_GPIO0->FIOSET = PIN_SPI_CLK;
    spi_delay();
    LPC_GPIO0->FIOCLR = PIN_SPI_CLK; // Le FPGA change son bit sur ce front descendant
    spi_delay();
}

static uint8_t LIRE_bit(void) {
    return (LPC_GPIO0->FIOPIN & PIN_SPI_MISO) ? 1 : 0;
}

/**
 * @brief Effectue la transaction SPI bit-bang pour un canal donné
 */
static void read_spi_channel(int cs_num) {
    uint32_t current_cs_pin = 0;
    uint32_t raw_data = 0;

    switch (cs_num) {
        case 0: current_cs_pin = PIN_SPI_CS1; break; // Vg
        case 1: current_cs_pin = PIN_SPI_CS2; break; // Vd
        case 2: current_cs_pin = PIN_SPI_CS3; break; // Pg
        case 3: current_cs_pin = PIN_SPI_CS4; break; // Pd
        default: return;
    }

    LPC_GPIO0->FIOCLR = PIN_SPI_CLK;
    spi_delay();
    LPC_GPIO0->FIOCLR = current_cs_pin; // Activation CS (Niveau bas)
    spi_delay(); 

    for (int i = 0; i < 32; i++) {
        if (LIRE_bit()) {
            raw_data |= (1U << i);
        }
        TOP_CLK();
    }

    LPC_GPIO0->FIOSET = current_cs_pin; // Désactivation CS (Niveau haut)
    spi_delay();

    // Stockage et masquage selon la taille utile définie
    switch (cs_num) {
        case 0: g_Vg = raw_data & 0x0FFFFFFF; break;
        case 1: g_Vd = raw_data & 0x0FFFFFFF; break;
        case 2: g_Pg = raw_data & 0x000000FF; break;
        case 3: g_Pd = raw_data & 0x000000FF; break;
    }
}

static float step_to_distance(uint32_t steps) {
    return (float)steps * DISTANCE_PER_STEP;
}

static float calc_filtered_velocity(uint32_t raw_velocity, uint32_t *history) {
    // Moyenne glissante sur 3 valeurs
    history[0] = history[1];
    history[1] = history[2];
    history[2] = raw_velocity;
    
    float avg_raw = (float)(history[0] + history[1] + history[2]) / 3.0f;
    
    // Calcul des grandeurs physiques
    float steps_per_second = avg_raw / FPGA_WINDOW_SEC; 
    float rev_per_second   = steps_per_second / ENCODER_RESOLUTION;
    
    return rev_per_second * WHEEL_PERIMETER;
}

/* ==========================================================================
 * FONCTIONS LOGIQUES ET INTERRUPTIONS
 * ========================================================================== */

void ODO_Init(void) {
    // Configuration des directions GPIO Port 0
    LPC_GPIO0->FIODIR |= (PIN_SPI_CLK | PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4);
    LPC_GPIO0->FIODIR &= ~PIN_SPI_MISO;

    // États initiaux (CS désactivés à '1', CLK au repos à '0')
    LPC_GPIO0->FIOSET = (PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4);
    LPC_GPIO0->FIOCLR = PIN_SPI_CLK;

    // Configuration P2.11 pour l'interruption externe EINT1 (Signal Top50 du FPGA)
    LPC_PINCON->PINSEL4 &= ~(3 << 22);
    LPC_PINCON->PINSEL4 |=  (1 << 22);
    LPC_SC->EXTMODE    |= (1 << 1);  // Sensible sur front
    LPC_SC->EXTPOLAR   |= (1 << 1);  // Front montant
    LPC_SC->EXTINT      = (1 << 1);  // Clear initial flag

    // Activation de l'interruption dans le NVIC (Priorité élevée pour l'asservissement)
    NVIC_SetPriority(EINT1_IRQn, 2);
    NVIC_EnableIRQ(EINT1_IRQn);
}

/**
 * @brief Routine de réception appelée par EINT1_IRQHandler (Déclenché 50 fois par seconde par le FPGA)
 */
void recep_spi_interrupt_routine(void) {
    // 1. Lecture séquentielle via le bus SPI simulé
    read_spi_channel(0); // Vg
    read_spi_channel(1); // Vd
    read_spi_channel(2); // Pg
    read_spi_channel(3); // Pd
    
    // 2. Traitement des données et écriture dans la structure partagée
    odo_shared_metrics.distance_g    = step_to_distance(g_Pg);
    odo_shared_metrics.distance_d    = step_to_distance(g_Pd);
    odo_shared_metrics.vitesse_g     = calc_filtered_velocity(g_Vg, hist_vg);
    odo_shared_metrics.vitesse_d     = calc_filtered_velocity(g_Vd, hist_vd);
    odo_shared_metrics.vitesse_robot = (odo_shared_metrics.vitesse_g + odo_shared_metrics.vitesse_d) / 2.0f;
    odo_shared_metrics.data_ready    = true;
}

void ODO_Get_Data(odometrie_data_t *data) {
    if (data ==  0) return;
    
    // Désactivation temporaire de l'interruption pour éviter une condition de concurrence (lecture atomique)
    NVIC_DisableIRQ(EINT1_IRQn);
    
    data->distance_g    = odo_shared_metrics.distance_g;
    data->distance_d    = odo_shared_metrics.distance_d;
    data->vitesse_g     = odo_shared_metrics.vitesse_g;
    data->vitesse_d     = odo_shared_metrics.vitesse_d;
    data->vitesse_robot = odo_shared_metrics.vitesse_robot;
    
    NVIC_EnableIRQ(EINT1_IRQn);
}

bool ODO_Check_New_Data(void) {
    if (odo_shared_metrics.data_ready) {
        odo_shared_metrics.data_ready = false; // Consommé
        return true;
    }
    return false;
}
/**
 * @file recep_spi.c
 * @brief Fichier du module recep_spi.
 */

#include "recep_spi.h"

#include "LPC17xx.h"

#include "robot_state.h"

/* ==========================================================================
 * VARIABLES PRIVÉES
 * ========================================================================== */
static int buffer_val[8];
static int buffer_pos = 0;

static uint8_t last_cs[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static uint8_t pos_write_cs = 0;
static uint8_t pos_read_cs = 0;

/* ==========================================================================
 * PROTOTYPES DES FONCTIONS PRIVÉES
 * ========================================================================== */
static void init_spi_clk(void);
static void init_spi_miso(void);
static void init_spi_cs(void);

/* ==========================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ========================================================================== */

static void init_spi_clk(void) {
    // Interruption externe 1 (EINT1) sur P2.11
    LPC_PINCON->PINSEL4 &= ~(3u << 22);
    LPC_PINCON->PINSEL4 |= (1u << 22);
    
    // Front montant
    LPC_SC->EXTMODE |= (1 << 1);
    LPC_SC->EXTPOLAR |= (1 << 1);
    
    NVIC_EnableIRQ(EINT1_IRQn);
}

static void init_spi_sclk(void) {
    // GPIO OUT P0.7 pour SCLK (bit-bang)
    LPC_PINCON->PINSEL0 &= ~(3u << 14);
    LPC_GPIO0->FIODIR |= PIN_SPI_CLK;
}

static void init_spi_miso(void) {
    // GPIO IN P0.8
    LPC_PINCON->PINSEL0 &= ~(3 << 16);
    LPC_GPIO0->FIODIR &= ~PIN_SPI_MISO;
}

static void init_spi_cs(void) {
    // GPIO OUT P0.9, P0.10, P0.11, P0.15
    LPC_PINCON->PINSEL0 &= ~(3u << 18); // P0.9
    LPC_PINCON->PINSEL0 &= ~(3u << 20); // P0.10
    LPC_PINCON->PINSEL0 &= ~(3u << 22); // P0.11
    LPC_PINCON->PINSEL0 &= ~(3u << 30); // P0.15
    
    LPC_GPIO0->FIODIR |= PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4;
    LPC_GPIO0->FIOCLR = PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4;
}

void init_recep_spi(void) {
    init_spi_clk();
    init_spi_sclk();
    init_spi_miso();
    init_spi_cs();
}

void set_spi_cs_val(int cs_num) {
    // Ne pas écraser les demandes en cours
    if ((pos_write_cs + 1) % 8 == pos_read_cs) {
        return;
    }
    
    // Nouvelle demande
    if (cs_num == 0) {
        LPC_GPIO0->FIOSET = PIN_SPI_CS1;
        last_cs[pos_write_cs] = 0;
    } else if (cs_num == 1) {
        LPC_GPIO0->FIOSET = PIN_SPI_CS2;
        last_cs[pos_write_cs] = 1;
    } else if (cs_num == 2) {
        LPC_GPIO0->FIOSET = PIN_SPI_CS3;
        last_cs[pos_write_cs] = 2;
    } else if (cs_num == 3) {
        LPC_GPIO0->FIOSET = PIN_SPI_CS4;
        last_cs[pos_write_cs] = 3;
    }
    pos_write_cs = (pos_write_cs + 1) % 8;
}

void recep_spi_interrupt_routine(void) {
    // Acquitter le drapeau d'interruption EINT1
    LPC_SC->EXTINT |= (1 << 1);
    
    // Désactiver tous les CS
    LPC_GPIO0->FIOCLR = PIN_SPI_CS1 | PIN_SPI_CS2 | PIN_SPI_CS3 | PIN_SPI_CS4;
    
    // Lire seulement si on a envoyé une demande
    if (pos_write_cs == pos_read_cs) {
        return;
    }
    
    // Lire MISO
    int val = (LPC_GPIO0->FIOPIN & PIN_SPI_MISO) ? 1 : 0;
    
    buffer_val[buffer_pos] = val;
    buffer_pos++;
    
    if (buffer_pos == 8) {
        buffer_pos = 0;
        
        // Décoder la valeur obtenue (LSB d'abord ou MSB ?)
        int decoded_val = buffer_val[0] + 
                          2 * (buffer_val[1] + 
                          2 * (buffer_val[2] + 
                          2 * (buffer_val[3] + 
                          2 * (buffer_val[4] + 
                          2 * (buffer_val[5] + 
                          2 * (buffer_val[6] + 
                          2 * buffer_val[7]))))));
        
        // Mettre à jour l'état du robot
        uint8_t vg, vd, pg, pd;
        get_spi_variables(&vg, &vd, &pg, &pd);
        
        if (last_cs[pos_read_cs] == 0) {
            vg = decoded_val;
        } else if (last_cs[pos_read_cs] == 1) {
            vd = decoded_val;
        } else if (last_cs[pos_read_cs] == 2) {
            pg = decoded_val;
        } else if (last_cs[pos_read_cs] == 3) {
            pd = decoded_val;
        }
        
        set_spi_variables(vg, vd, pg, pd);
        
        pos_read_cs = (pos_read_cs + 1) % 8;
    }
}

void test_recep_spi(void) {
    // Test simple : demander la lecture de Vg (CS 0)
    set_spi_cs_val(0);
}
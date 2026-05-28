#include "LPC17xx.h"

extern uint8_t Vg, Vd, Pg, Pd;

// Les init
void initClk();
void initMISO();
void initCS1234();

// Demander un valeur pour un CS précis (0 à 3)
void setCSVal(int csNum);

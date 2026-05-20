#include "LPC17xx.h"

// Les fonctions d'initialisation
void initTimerRecepIR();
void initGPIOInteruptRecepIR();

// Interuption GPIO -> Appelle cette fonction
void interuptGPIORecepIR ();

extern int numeroRobot;
extern int vitesseRobot;
extern int statusRobot;

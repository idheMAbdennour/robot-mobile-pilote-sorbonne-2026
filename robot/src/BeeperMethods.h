#include "LPC17xx.h"


// Dans le main stp
void initBeepeur();

// Gestion des distance lié au beeper
void changeDist(int msecVal);
void stopBeep();
void startBeep(int msecVal);

// Quand on doit charger ou décharger (nbFois = 1 pour la décharge ou 2 pour la charge
void startBeepChargeDecharge(int nbFois);
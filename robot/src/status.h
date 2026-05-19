#include "LPC17xx.h"

void initLedChangementStatus();
void changementDeStatus(char nouveauStatus);
int mainTestStatusLED();

// GPIO P1.27 -> Rouge
// GPIO P1.28 -> Vert
// GPIO P1.29 -> Bleu
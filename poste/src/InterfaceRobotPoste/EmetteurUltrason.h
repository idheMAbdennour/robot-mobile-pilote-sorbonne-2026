#ifndef EMETTEUR_ULTRASON_H
#define EMETTEUR_ULTRASON_H

#include "LPC17xx.h"

void initUltrasonsPWM_Continuous(void);
void initUltrasonsEnvelope_GPIO(void);
void trigger_envelope_hardware(int numPoste, char coter);
void handle_ultrasons_envelope_non_blocking(void);

#endif
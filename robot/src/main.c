#include "status.h"
#include "dtmf.h"

int main() {
	// Status
	initLedChangementStatus();
	init_dtmf();
	//mainTestStatusLED(); // Décommenté pour tester le module
	
	while(1) {}
}

#include "LPC17xx.h"
#include "fil.h"

// Fonction de temporisation basique pour le test (Bloquante, juste pour la démo)
void Delay_ms(uint32_t ms) {
    // Ajustement approximatif de la boucle en fonction de la vitesse du CPU (ex: 100MHz ou 120MHz)
    volatile uint32_t count = ms * (SystemCoreClock / 10000);
    while(count--) {
        __NOP(); 
    }
}

int main(void) {
    // 1. Initialisation obligatoire du système (Configuration des horloges du LPC1769)
    SystemInit();
    
    // 2. Initialisation de notre générateur (GPIOs en sortie + Démarrage du TIMER0 à 10µs)
    Generator_Init();
    
    // Au démarrage, le buffer est vide. 
    // Le TIMER0 va automatiquement émettre E, e, E, e... en boucle.

    while(1) {
        // Attente de 1,5 seconde au repos (Visualisation des pulses de synchronisation E et e)
        Delay_ms(1500);
        
        // --- TEST CASE 1 : Envoi de l'exemple du cahier des charges ---
        // Demande R02V40 : Robot n°2 à 40% de sa vitesse
        // Type de message : 000 | Robot : 0010 | Paramètre : 0101000 (40)
        Generator_Cmd_Vitesse(2, 40);
        
        // Pendant l'envoi de ce message (qui dure environ 60 à 70 ms à cause de l'alternance),
        // la broche P0.8 (PIN_DEBUG) va passer à l'état HAUT, puis retomber à BAS dès la fin du message.
    }
}
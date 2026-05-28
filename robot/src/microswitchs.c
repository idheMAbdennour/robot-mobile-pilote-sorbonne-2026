#include "LPC17xx.h"
#include "microswitchs.h"
#include "robotState.h"

void init_microswitchs(void)
{
    // Configurer P0.29 et P0.30 en GPIO (PINSEL = 00)
    LPC_PINCON->PINSEL1 &= ~((3 << 26) | (3 << 28));

    // Configurer P0.29 et P0.30 en Entrée (FIODIR = 0)
    LPC_GPIO0->FIODIR &= ~((1 << 29) | (1 << 30));

    // Activer les interruptions sur fronts montants et descendants
    LPC_GPIOINT->IO0IntEnR |= (1 << 29) | (1 << 30);
    LPC_GPIOINT->IO0IntEnF |= (1 << 29) | (1 << 30);

    // Initialiser l'état au démarrage
    uint8_t sw1 = (LPC_GPIO0->FIOPIN >> 29) & 1;
    uint8_t sw2 = (LPC_GPIO0->FIOPIN >> 30) & 1;
    set_microswitch_state((sw2 << 1) | sw1);
    
    // NB: L'interruption globale EINT3_IRQn est déjà activée par les autres capteurs
}

void microswitchs_interrupt_routine(void)
{
    // Vérifier si l'interruption vient de P0.29 ou P0.30
    if (LPC_GPIOINT->IO0IntStatR & ((1 << 29) | (1 << 30)) ||
        LPC_GPIOINT->IO0IntStatF & ((1 << 29) | (1 << 30)))
    {
        // Lire l'état actuel
        uint8_t sw1 = (LPC_GPIO0->FIOPIN >> 29) & 1;
        uint8_t sw2 = (LPC_GPIO0->FIOPIN >> 30) & 1;
        
        // Mettre à jour l'état dans le robotState
        set_microswitch_state((sw2 << 1) | sw1);
        
        // Acquitter les interruptions
        LPC_GPIOINT->IO0IntClr = (1 << 29) | (1 << 30);
    }
}
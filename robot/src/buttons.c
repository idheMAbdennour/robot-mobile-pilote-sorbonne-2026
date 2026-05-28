#include "LPC17xx.h"
#include "robotState.h"
#include "status.h"
#include "buttons.h"
#include "stepper.h"

#define BTN_CHARGE (1<<4)
#define BTN_DECHARGE (1<<5)

void buttons_init(void){
    LPC_PINCON->PINSEL4 &= ~(3<<8);
    LPC_PINCON->PINSEL4 &= ~(3<<10);
    LPC_GPIO2->FIODIR &= ~(1<<4);
    LPC_GPIO2->FIODIR &= ~(1<<5);
}

void buttons_check(void){
    if (!(LPC_GPIO2->FIOPIN) & (1<<4)){
        set_robot_status(STATUS_COLISPRIS);
        changementDeStatus('C');
    }
    if (!(LPC_GPIO2->FIOPIN) & (1<<5)){
        set_robot_status(STATUS_LIBRE);
        changementDeStatus('L');
        stepper_move_to(POS_EMPTY);
    }
}
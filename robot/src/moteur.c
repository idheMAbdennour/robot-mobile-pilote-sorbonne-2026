#include "LPC17xx.h"
#include <stdint.h>
#include <stdio.h>
#include "robotState.h"
#include "uart.h"

#define PWM_PERIOD_TICKS 4000

#define PIN_MOT_SW1 (1 << 11) 
#define PIN_MOT_SW2 (1 << 12) 

static uint8_t mot_hw_mode = 0b11;
static uint8_t mot_wire_pending = 0;
static uint8_t mot_wire_mode = 0;

static void init_moteurs_switches(void);
static void moteurs_update_mode_from_gpio(void);
static void moteurs_send_frame(uint8_t mode, const char *prefix);

void Init_Moteur_PWM(void) {
    LPC_SC->PCONP |= (1 << 6);

    LPC_SC->PCLKSEL0 &= ~(3 << 12);
    LPC_SC->PCLKSEL0 |= (1 << 12);

    LPC_PINCON->PINSEL4 &= ~0x3F;   
    LPC_PINCON->PINSEL4 |= 0x14;    

    LPC_PWM1->PR = 0;
    LPC_PWM1->MR0 = PWM_PERIOD_TICKS; 

    LPC_PWM1->MR2 = 0;             
    LPC_PWM1->MR3 = 0;             

    LPC_PWM1->LER = (1 << 0) | (1 << 2) | (1 << 3);

    LPC_PWM1->PCR = (1 << 10) | (1 << 11); 

    LPC_PWM1->TCR = (1 << 1); 
    for(volatile uint32_t i = 0; i < 10; i++);
    LPC_PWM1->TCR = (1 << 0) | (1 << 3); 
    
    set_motor_pwms(0, 0);
}

void init_moteurs_debug(void)
{
    init_moteurs_switches();
    moteurs_update_mode_from_gpio();
    NVIC_EnableIRQ(EINT3_IRQn);
}

static void init_moteurs_switches(void)
{
    LPC_PINCON->PINSEL0 &= ~((3u << 22) | (3u << 24));
    LPC_GPIO0->FIODIR &= ~(PIN_MOT_SW1 | PIN_MOT_SW2);

    LPC_GPIOINT->IO0IntEnR |= (PIN_MOT_SW1 | PIN_MOT_SW2);
    LPC_GPIOINT->IO0IntEnF |= (PIN_MOT_SW1 | PIN_MOT_SW2);
}

static void moteurs_update_mode_from_gpio(void)
{
    uint8_t sw1 = LPC_GPIO0->FIOPIN & PIN_MOT_SW1;
    uint8_t sw2 = LPC_GPIO0->FIOPIN & PIN_MOT_SW2;

    mot_hw_mode = (sw2 << 1) | sw1;
}

void Changer_PWM_Gauche(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;

    LPC_PWM1->MR2 = (PWM_PERIOD_TICKS * pourcent) / 100;
    LPC_PWM1->LER |= (1 << 2);
    
    int32_t dummy, p_droite;
    get_motor_pwms(&dummy, &p_droite);
    set_motor_pwms((int32_t)pourcent, p_droite);
}

void Changer_PWM_Droite(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;

    LPC_PWM1->MR3 = (PWM_PERIOD_TICKS * pourcent) / 100;
    LPC_PWM1->LER |= (1 << 3);
    
    int32_t p_gauche, dummy;
    get_motor_pwms(&p_gauche, &dummy);
    set_motor_pwms(p_gauche, (int32_t)pourcent);
}

void Changer_PWM_Moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite) {
    if (pourcent_gauche > 100) pourcent_gauche = 100;
    if (pourcent_droite > 100) pourcent_droite = 100;

    LPC_PWM1->MR2 = (PWM_PERIOD_TICKS * pourcent_gauche) / 100;
    LPC_PWM1->MR3 = (PWM_PERIOD_TICKS * pourcent_droite) / 100;

    LPC_PWM1->LER |= (1 << 2) | (1 << 3);
    
    set_motor_pwms((int32_t)pourcent_gauche, (int32_t)pourcent_droite);
}

void moteurs_interrupt_routine(void)
{
    if (LPC_GPIOINT->IO0IntStatR & (PIN_MOT_SW1 | PIN_MOT_SW2) |
        LPC_GPIOINT->IO0IntStatF & (PIN_MOT_SW1 | PIN_MOT_SW2))
    {
        moteurs_update_mode_from_gpio();
        LPC_GPIOINT->IO0IntClr = (PIN_MOT_SW1 | PIN_MOT_SW2);
    }
}

void moteurs_receive_wire_command(uint8_t wire_code)
{
    if (wire_code == 0b100 || wire_code == 0b101) {
        mot_wire_mode = wire_code & 0x1u;
        mot_wire_pending = 1;
    }
}

static void moteurs_send_frame(uint8_t mode, const char *prefix)
{
    char buffer[96];
    int offset = 0;
    int32_t pwm_g = 0;
    int32_t pwm_d = 0;
    int32_t v_moy = 0;
    int32_t w_ang = 0;

    if (prefix) {
        offset += sprintf(buffer + offset, "%s", prefix);
    }

    get_motor_pwms(&pwm_g, &pwm_d);
    get_motor_speeds(&v_moy, &w_ang);

    switch (mode)
    {
        case 0b00:
            offset += sprintf(buffer + offset, "W %lddeg/s\r\n", (long)w_ang);
            break;
        case 0b01:
            offset += sprintf(buffer + offset, "V %ldcm /s\r\n", (long)v_moy);
            break;
        case 0b10:
            offset += sprintf(buffer + offset, "G %ld D %ld\r\n", (long)pwm_g, (long)pwm_d);
            break;
        default:
            buffer[0] = '\0';
            break;
    }

    if (buffer[0] != '\0') {
        uart0_send_string(buffer);
    }
}

void debug_moteurs_send_frame(void)
{
    if (mot_hw_mode == 0b11)
    {
        if (!mot_wire_pending) {
            return;
        }

        moteurs_send_frame(mot_wire_mode, "wire overwrite ");
        mot_wire_pending = 0;
        return;
    }

    moteurs_send_frame(mot_hw_mode, NULL);
}
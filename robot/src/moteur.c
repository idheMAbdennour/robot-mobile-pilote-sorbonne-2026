#include "LPC17xx.h"
#include <stdint.h>
#include <stdio.h>
#include "robotState.h"
#include "uart.h"

// On d�finit la valeur maximale du compteur pour une p�riode de 40�s (� 100 MHz)
#define PWM_PERIOD_TICKS 4000

#define PIN_MOT_SW1 (1 << 11) // P0.11
#define PIN_MOT_SW2 (1 << 12) // P0.12

static uint8_t mot_hw_mode = 0b11;
static uint8_t mot_wire_pending = 0;
static uint8_t mot_wire_mode = 0;

/* Forward declarations for static helpers used before their definitions */
static void init_moteurs_switches(void);
static void moteurs_update_mode_from_gpio(void);
static void moteurs_send_frame(uint8_t mode, const char *prefix);

/**
 * @brief Initialise le module PWM1 pour les deux roues du robot [cite: 174]
 * Utilise les broches P2.0 (PWM1.1) pour la roue gauche et P2.1 (PWM1.2) pour la roue droite.
 */
void Init_Moteur_PWM(void) {
    // 1. Activation de l'alimentation du bloc PWM1
    LPC_SC->PCONP |= (1 << 6);

    // 2. Configuration de l'horloge du PWM1 (PCLK_PWM1 = CCLK = 100 MHz)
    LPC_SC->PCLKSEL0 &= ~(3 << 12);
    LPC_SC->PCLKSEL0 |= (1 << 12);

    // 3. Configuration des broches P2.0 et P2.1 en mode PWM1.1 et PWM1.2
    LPC_PINCON->PINSEL4 &= ~0xF;   // Reset des fonctionnalit�s de P2.0 et P2.1
    LPC_PINCON->PINSEL4 |= 0x5;    // S�lectionne la fonction "PWM" (01) pour ces deux broches

    // 4. Configuration du compteur (Pas de division d'horloge)
    LPC_PWM1->PR = 0;

    // 5. D�finition de la p�riode globale (< 50�s pour le silence )
    LPC_PWM1->MR0 = PWM_PERIOD_TICKS; // 4000 ticks = 40 �s (Fr�quence de 25 kHz)

    // 6. Extinction initiale des moteurs (Rapport cyclique � 0%)
    LPC_PWM1->MR1 = 0;             // Canal 1 (Roue Gauche)
    LPC_PWM1->MR2 = 0;             // Canal 2 (Roue Droite)

    // 7. Demande de prise en compte imm�diate des valeurs (Latch Enable)
    LPC_PWM1->LER = (1 << 0) | (1 << 1) | (1 << 2);

    // 8. Activation des sorties PWM1.1 et PWM1.2 en mode simple rampe
    LPC_PWM1->PCR = (1 << 9) | (1 << 10);

    // 9. D�marrage du compteur principal et activation du mode PWM
    LPC_PWM1->TCR = (1 << 0) | (1 << 3);
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


/**
 * @brief Modifie le rapport cyclique de la roue GAUCHE � tout moment
 * @param pourcent Valeur entre 0 (arr�t) et 100 (vitesse max)
 */
void Changer_PWM_Gauche(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;

    // Calcul de la correspondance en ticks pour le registre de comparaison
    LPC_PWM1->MR1 = (PWM_PERIOD_TICKS * pourcent) / 100;

    // CRUCIAL : On active le "Latch" pour que la modification soit appliqu�e
    // proprement au d�but du prochain cycle PWM, sans cr�er de glitch �lectrique.
    LPC_PWM1->LER |= (1 << 1);
}

/**
 * @brief Modifie le rapport cyclique de la roue DROITE � tout moment
 * @param pourcent Valeur entre 0 (arr�t) et 100 (vitesse max)
 */
void Changer_PWM_Droite(uint8_t pourcent) {
    if (pourcent > 100) pourcent = 100;

    // Calcul de la correspondance en ticks
    LPC_PWM1->MR2 = (PWM_PERIOD_TICKS * pourcent) / 100;

    // Activation du Latch pour le canal 2
    LPC_PWM1->LER |= (1 << 2);
}

/**
 * @brief Modifie les deux roues simultan�ment (Pratique pour ton asservissement)
 */
void Changer_PWM_Moteurs(uint8_t pourcent_gauche, uint8_t pourcent_droite) {
    if (pourcent_gauche > 100) pourcent_gauche = 100;
    if (pourcent_droite > 100) pourcent_droite = 100;

    LPC_PWM1->MR1 = (PWM_PERIOD_TICKS * pourcent_gauche) / 100;
    LPC_PWM1->MR2 = (PWM_PERIOD_TICKS * pourcent_droite) / 100;

    // On valide les deux canaux en m�me temps
    LPC_PWM1->LER |= (1 << 1) | (1 << 2);
}


void moteurs_interrupt_routine(void)
{
    if (LPC_GPIOINT->IO0IntStatR & (PIN_MOT_SW1 | PIN_MOT_SW2) ||
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

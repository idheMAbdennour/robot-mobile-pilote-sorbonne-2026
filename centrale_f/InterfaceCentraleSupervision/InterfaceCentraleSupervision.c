#include "InterfaceCentraleSupervision.h"

#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 128

static char sup_rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t sup_rx_index = 0;

static char tx_buffer[TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

static uint32_t last_tx_tick = 0;
extern volatile uint32_t tick_ms;

void Init_Supervision_UART0(uint32_t baudrate) {
    LPC_SC->PCONP |= (1 << 3);

    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |=  ((1 << 4) | (1 << 6));  

    LPC_UART0->LCR = (1 << 7) | (3 << 0);

    uint32_t pclk = SystemCoreClock / 4;
    uint32_t d = pclk / (16 * baudrate);
    LPC_UART0->DLM = (d >> 8) & 0xFF;
    LPC_UART0->DLL = d & 0xFF;

    LPC_UART0->LCR &= ~(1 << 7);
    LPC_UART0->FCR = (1 << 0) | (1 << 1) | (1 << 2);
}

void Supervision_Push_TX_Queue(const char *str) {
    while (*str) {
        uint16_t next = (tx_head + 1) % TX_BUFFER_SIZE;
        if (next != tx_tail) {
            tx_buffer[tx_head] = *str;
            tx_head = next;
        }
        str++;
    }
}

void Supervision_Process_TX_Non_Blocking(void) {
    if (tx_tail == tx_head) return; 

    if (LPC_UART0->LSR & (1 << 5)) { 
        LPC_UART0->THR = tx_buffer[tx_tail];
        tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
    }
}

void Supervision_Forward_Poste_Message(uint8_t post_id, const char *raw_msg) {
    char fwd_buffer[32];
    sprintf(fwd_buffer, "P%02d%s\r\n", post_id, raw_msg);
    Supervision_Push_TX_Queue(fwd_buffer); 
}

static void parse_supervision_cmd(const char *cmd) {
    int r_id = 0, vitesse = 0, p_dep = 0, p_arr = 0;
    char service_letter = 'A';

    if (cmd[0] != 'R') return;

    if (strchr(cmd, 'V') != NULL) {
        if (sscanf(cmd, "R%dV%d", &r_id, &vitesse) == 2) {
            if (r_id < MAX_ROBOTS) {
                robots_db[r_id].robot_id = r_id;
                robots_db[r_id].vitesse_voulue = vitesse; 
            }
        }
    }
    else if (strchr(cmd, '-') != NULL) {
        if (sscanf(cmd, "R%dP%d-P%d", &r_id, &p_dep, &p_arr) == 3) {
            if (r_id < MAX_ROBOTS) {
                robots_db[r_id].enlevement = p_dep;
                robots_db[r_id].livraison = p_arr;
            }
        }
    }
    else {
        if (sscanf(cmd, "R%dP%d%cP%d", &r_id, &p_dep, &service_letter, &p_arr) == 4) {
            if (r_id < MAX_ROBOTS) {
                robots_db[r_id].enlevement = p_dep;
                robots_db[r_id].livraison = p_arr;
            }
        }
    }
}

void Supervision_Process_Incoming_Non_Blocking(void) {
    if (!(LPC_UART0->LSR & (1 << 0))) return;

    char c = LPC_UART0->RBR;

    if (c == '\r' || c == '\n') {
        if (sup_rx_index > 0) {
            sup_rx_buffer[sup_rx_index] = '\0';
            parse_supervision_cmd(sup_rx_buffer);
            sup_rx_index = 0;
        }
    } else {
        if (sup_rx_index < (RX_BUFFER_SIZE - 1)) {
            sup_rx_buffer[sup_rx_index++] = c;
        } else {
            sup_rx_index = 0;
        }
    }
}

void Supervision_Manage_Wire_Transmission_Tick(void) {
    if ((uint32_t)(tick_ms - last_tx_tick) < 200) return;
    last_tx_tick = tick_ms;

    for (uint8_t id = 1; id < MAX_ROBOTS; id++) {
        if (robots_db[id].robot_id == id) {
            if (robots_db[id].vitesse_voulue != robots_db[id].vitesse_actuelle) {
                Generator_Cmd_Vitesse(id, robots_db[id].vitesse_voulue);
                return;
            }

            if (robots_db[id].enlevement != 0) {
                uint8_t type_msg = (robots_db[id].enlevement % 2 == 0) ? 0x02 : 0x01;
                Generator_Cmd_Station(type_msg, id, 0x00, robots_db[id].enlevement);
                return;
            }
            if (robots_db[id].livraison != 0 && robots_db[id].status == 'C') {
                uint8_t type_msg = (robots_db[id].livraison % 2 == 0) ? 0x04 : 0x03;
                Generator_Cmd_Station(type_msg, id, 0x00, robots_db[id].livraison);
                return;
            }
        }
    }
}
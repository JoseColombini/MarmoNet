#ifndef _BS_GLOBAL
#define _BS_GLOBAL

#include "../marmonet_structs.h"

extern event_queue_t _eq;
extern event_t _update_evt;
extern event_timeout_t _update_timeout_evt;

extern uint16_t conected_handler;

extern MarmoNet_BSData Data;

typedef uint8_t mac_addr[6];

extern const mac_addr node_addr[3];

typedef struct
{
    uint16_t lat_handler;
    uint16_t data_handler;
    uint16_t sync_handler;
    uint16_t status_handler;
    uint16_t maks_handler;
    
} chrs;

extern chrs chrs_list;

extern const struct ble_gap_conn_params ble_gap_conn_params_dflt;

extern bool syncing;

#endif
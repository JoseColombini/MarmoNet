#ifndef _NODE_GLOBAL
#define _NODE_GLOBAL

// #include "../marmonet_structs.h"
#include "../marmonet_helpers.h"

#include "../bmx_simple.h"


/**
 * Event configurations
*/
extern event_queue_t _eq;
extern event_t _update_evt;
extern event_timeout_t _update_timeout_evt;
/**
 * MarmoNet Configuration
*/


// MarmoNet_NodeWakeup wakeup;

extern uint8_t encounters;
extern uint8_t encounters_fails;

extern MarmoNet_CallithrixData Data;

//pattern to read adv
extern uint8_t key_ptr[];


extern bool con;
extern uint16_t _conn_handle;


extern uint32_t timer;
extern uint8_t last_sync;
extern bool fail_safe;






#endif
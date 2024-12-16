#ifndef _MARMONET_STRUCTS_H
#define _MARMONET_STRUCTS_H

#include "MarmoNet_params.h"
#include <stdint.h>
#include "nimble_riot.h"
#include "nimble_autoadv.h"
#include "nimble_scanner.h"

#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "net/ble.h"
#include "event/timeout.h"

#include <stdint.h>

//ID for animal identification
typedef enum _MarmoNet__ID {
  MARMONET_ID_COLOMBINI = 0b00000001,
  MARMONET_ID_LOUISE    = 0b00000010,
  MARMONET_ID_ISA       = 0b00000100,
  MARMONET_ID_BRUNO     = 0b00001000,
  MARMONET_ID_PEDRO     = 0b00010000,
  MARMONET_ID_LUIZA     = 0b00100000,
  MARMONET_ID_MARCIO    = 0b01000000,
  MARMONET_ID_MAMACO    = 0b10000000,
} MarmoNet_ID;


//Masks to activate or deactivate a sensor
typedef enum {

  MARMONET_MASK_ID          = 0b00000001,
  MARMONET_MASK_PRESSURE    = 0b00000010,
  MARMONET_MASK_HUMIDITY    = 0b00000100,
  MARMONET_MASK_TEMPERATURE = 0b00001000,
  MARMONET_MASK_LUMENS      = 0b00010000,

}DATA_MASK;




typedef struct 
{
  #if USE_BAROMETER
    uint32_t barometer;
  #endif
  #if USE_TEMPERATURE
    int16_t temperature;
  #endif
  #if USE_HUMIDITY
    uint16_t humidity;
  #endif
} bmx_data;

typedef struct _MarmoNet__NodeWakeup MarmoNet_NodeWakeup;

typedef struct _MarmoNet_Event{

  uint32_t event_n;
  uint8_t neighbors_id;
  uint8_t fail_safe_found;
#if USE_BMX
  bmx_data enviroment;
#endif
  uint8_t mask;


}MarmoNet_Event;

#define MAX_EVENT_SEND  MAXPAYLOAD_SIZE/sizeof(MarmoNet_Event)


struct  _MarmoNet__NodeWakeup
{
 
  MarmoNet_Event event;

  MarmoNet_NodeWakeup* stack_wakeup;
};

typedef struct  _MarmoNet__NodeInfo
{
  uint8_t my_id;
  uint16_t n_wakeup;
  uint8_t not_sent_wakeup;
  uint8_t current_mask;
  uint16_t last_sync;

}MarmoNet_NodeInfo;

typedef struct  _MarmoNet__CallithrixData
{
  MarmoNet_NodeInfo info;

  MarmoNet_NodeWakeup* stack_head_wakeup;
}MarmoNet_CallithrixData;


typedef struct
{
  MarmoNet_NodeInfo abi_info;
  uint16_t bs_event_n;
  bmx_data bs_enviroment;

  uint8_t stack_size;

  MarmoNet_NodeWakeup* stack_head_wakeup;

}MarmoNet_data_recover;


typedef struct _MarmoNet__BS_Collection MarmoNet_BS_Collection;
typedef struct _MarmoNet__BS_Collection
{
  MarmoNet_data_recover data;

  MarmoNet_BS_Collection* stack_collection;

};

typedef struct 
{
  MarmoNet_NodeInfo info;

  MarmoNet_BS_Collection* stack_head_collection;

}MarmoNet_BSData;


typedef struct 
{
  ztimer_now_t timer;
  uint16_t current_wakeup;
}sync_data;


#endif
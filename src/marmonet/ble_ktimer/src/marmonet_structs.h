#ifndef _MARMONET_STRUCTS_H
#define _MARMONET_STRUCTS_H

#include "marmonet_params.h"

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


struct bme280_reading {
	/* Compensated values. */
	int32_t comp_temp;
	uint32_t comp_press;
	uint32_t comp_humidity;
};

struct bme280_data {
	/* Compensation parameters. */
	uint16_t dig_t1;
	int16_t dig_t2;
	int16_t dig_t3;
	uint16_t dig_p1;
	int16_t dig_p2;
	int16_t dig_p3;
	int16_t dig_p4;
	int16_t dig_p5;
	int16_t dig_p6;
	int16_t dig_p7;
	int16_t dig_p8;
	int16_t dig_p9;
	uint8_t dig_h1;
	int16_t dig_h2;
	uint8_t dig_h3;
	int16_t dig_h4;
	int16_t dig_h5;
	int8_t dig_h6;

	/* Carryover between temperature and pressure/humidity compensation. */
	int32_t t_fine;

	uint8_t chip_id;

	struct bme280_reading reading;
};

// typedef struct 
// {
//     uint32_t pression;
//     int32_t temperature;
//     uint32_t humidity;

// } bme280_reading;

typedef struct _MarmoNet__Event__stack MarmoNet_Event_stack;

typedef struct _MarmoNet_Event{

  uint32_t event_n;
  uint8_t neighbors_id;
  uint8_t fail_safe_found;
#if USE_BMX
  struct bme280_reading enviroment;
#endif
  uint8_t mask;

}MarmoNet_Event;

#define MAX_EVENT_SEND  MAXPAYLOAD_SIZE/sizeof(MarmoNet_Event)


struct  _MarmoNet__Event__stack
{
 
  MarmoNet_Event event;

  MarmoNet_Event_stack* stack_wakeup;
};

typedef struct  _MarmoNet__NodeInfo
{
  uint8_t my_id;  //ID of the node
  uint16_t n_wakeup; //How many data were collected
  uint8_t not_sent_wakeup; //the nmber of data that were not recovered (work as index for the data array)
  uint8_t current_mask; //The current mask of data to be collected
  uint16_t last_sync; //Last time the node was synchronized

}MarmoNet_NodeInfo;

typedef struct  _MarmoNet__CallithrixData
{
  MarmoNet_NodeInfo info;

  MarmoNet_Event wakeup_data[100];
}MarmoNet_CallithrixData;


typedef struct
{
  MarmoNet_NodeInfo abi_info;
  uint16_t bs_event_n;
  struct bme280_reading bs_enviroment;

  uint8_t stack_size;

  MarmoNet_Event_stack* stack_head_wakeup;

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

  MarmoNet_data_recover data_recovered[100];

}MarmoNet_BSData;



#endif
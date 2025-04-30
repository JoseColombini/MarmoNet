#ifndef MARMONET_HELPERS_H
#define MARMONET_HELPERS_H

#include <zephyr/drivers/sensor.h>
#include "marmonet_structs.h"

int init_bme280();

int fetch_bme280();
int32_t get_temperature();
uint32_t get_pressure();
uint32_t get_humidity();

// static void read_sensor_values(bmx_data* data);


#endif
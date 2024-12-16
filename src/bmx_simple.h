#ifndef _BMX_SIMPLE
#define _BMX_SIMPLE

#include "bmx280_params.h"
#include "bmx280.h"


int bmx_simple_init();


int16_t get_temperature();


uint32_t get_pressure();


uint16_t get_humidity();


#endif

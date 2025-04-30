#ifndef SENSORS_INTERFACE_H
#define SENSORS_INTERFACE_H

#include "marmonet_params.h"


/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
// #define SENSORS_THREAD_STACK_SIZE 1024
// #define SENSORS_THREAD_PRIORITY 5 /* preemptible */

// Encoding used to map sensors APIs
// **Sensors must be on the same order as in DataType enum**
enum SensorType
{
	#if USE_BMX
		BME280,
	#endif
	#if USE_BMI
		BMI160,
	#endif
	#if USE_SI1133
		SI1133,
	#endif
	#if USE_SCD30
		SCD30,
	#endif
	MAX_SENSORS // Total number of sensors
};

// Functions exposed for each sensor
typedef struct
{
	// Initializes sensor
	int (*init_sensor)();
	// Reads sensor values and stores them in buffer
	void (*read_sensor_values)();
	// Data processing API
	// DataAPI *data_model_api;
} SensorAPI;

// List of registered sensor APIs
extern SensorAPI *sensor_apis[MAX_SENSORS];

// Registers callbacks for the used sensors
int register_sensors_callbacks();
// Initializes sensors and start reading them
int read_sensors();


// #TODO: probably will require sync
// Dynamically sets current sampling interval
// void set_sampling_interval(int new_interval);
// Gets current sampling interval
// int get_sampling_interval();

#endif /* SENSORS_INTERFACE_H */
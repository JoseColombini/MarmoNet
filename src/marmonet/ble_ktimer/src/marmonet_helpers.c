#include "marmonet_helpers.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

/**
 * IMPLEMENTATIONS
 */
LOG_MODULE_REGISTER(helpers, LOG_LEVEL_DBG);


const struct device *bme280;

// Gets and initializes device
int init_bme280()
{
    LOG_DBG("Initializing BME280");
    bme280 = DEVICE_DT_GET_ANY(bosch_bme280);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!bme280)
    {
        LOG_ERR("bme280 not declared at device tree");
        return -1;
    }
    else if (!device_is_ready(bme280))
    {
        LOG_ERR("device \"%s\" is not ready", bme280->name);
        return -1;
    }
    return 0;
}

int32_t get_temperature()
{
    struct bme280_data* data = bme280->data;
    LOG_DBG("Temperature: %d", data->reading.comp_temp);
    return data->reading.comp_temp;
}

uint32_t get_pressure()
{
    struct bme280_data* data = bme280->data;
    LOG_DBG("Pressure: %d", data->reading.comp_press);
    return data->reading.comp_press;
}

uint32_t get_humidity()
{
    struct bme280_data* data = bme280->data;
    LOG_DBG("Humidity: %d", data->reading.comp_humidity);
    return data->reading.comp_humidity;
}

int fetch_bme280()
{
sample_fetch:
    int error = sensor_sample_fetch(bme280);
    if(!error)
        return 0;
    if (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
                bme280->name, error);
        k_sleep(K_MSEC(1));
        goto sample_fetch;
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                bme280->name, error);
    }
    return 0;
}

// static void read_sensor_values(bmx_data* data)
// {
//     LOG_DBG("Reading BME280");


// sample_fetch:
//     int error = sensor_sample_fetch(bme280);
//     if (!error)
//     {
//         sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP,
//                            *data.temperature);
//         sensor_channel_get(bme280, SENSOR_CHAN_PRESS,
//                            *data.pressure);
//         sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY,
//                            *data.humidity);

//         memcpy(&bme280_data, &bme280_model, sizeof(SensorModelBME280));

//         if (insert_in_buffer(&app_buffer, bme280_data, BME280_MODEL, error, BME280_MODEL_WORDS) != 0)
//         {
//             LOG_ERR("Failed to insert data in ring buffer.");
//         }
//     }
//     else if (error == -EAGAIN)
//     {
//         LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
//                 bme280->name, error);
//         goto sample_fetch;
//     }
//     else
//     {
//         LOG_ERR("fetch sample from \"%s\" failed: %d",
//                 bme280->name, error);
//     }
// }


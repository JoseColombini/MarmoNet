#include "../bmx_simple.h"

bmx280_t bmx;

int bmx_simple_init()
{
        switch (bmx280_init(&bmx, &bmx280_params[0]))
        {
        case BMX280_ERR_BUS:
            puts("[Error] Something went wrong when using the I2C bus");
            return 1;
        case BMX280_ERR_NODEV:
            puts("[Error] Unable to communicate with any BMX280 device");
            return 1;
        default:
            /* all good -> do nothing */
            break;
        }
        return 0;
}

int16_t get_temperature()
{
    return bmx280_read_temperature(&bmx);
}

uint32_t get_pressure()
{
    return bmx280_read_pressure(&bmx);
}

uint16_t get_humidity()
{
    return bme280_read_humidity(&bmx);
}



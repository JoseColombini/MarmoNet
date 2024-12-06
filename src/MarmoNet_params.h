#ifndef _MARMONET_PARAMS_H
#define _MARMONET_PARAMS_H

/*
    CONFIGURATION PARAMETERS
*/

/*
    TIMER AND WAKEUP CYCLES
*/


#define WAKEUP_PERIOD       32*MSEC_PER_SEC //SECONDS
#define DUTY_CYCLE          50/100 //in PERCENT
// #define DRIFT_TOLERANCE     10/100 //PERCENT
#define TOLERANCE_TIME_ZONE 0  // TODO THIS SHOULD BE IN PERCENTAGE BUT IT IS HARDOCDED FOR NOWWAKEUP_PERIOD*DUTY_CYLCE*DRIFT_TOLERANCE
#define LOOPS               1 //number of times each node will adv (create a loop for round robin)
#define MAX_NODES           2 //Number of nodes in the sysmte
#define TURN_DURATION       (WAKEUP_PERIOD*DUTY_CYCLE - TOLERANCE_TIME_ZONE)/(LOOPS*MAX_NODES)

//For now we are using a single timer type so this is a sequence in a row
#define NMBR_WKPS_SEQ 10     

#define LONG_SLEEP 10*WAKEUP_PERIOD


/*
    FAIL SAFE CONFIG
*/
#define USE_FAIL_SAFE       0
#define WAKEUP_FAIL_SAFE    10*MSEC_PER_SEC
#define FAIL_SAFE_ADV_TIMER 5*MSEC_PER_SEC

/*
    SENSORS CONFIGURATIONS
*/

#define USE_BMX          1 //0 do not use BMX 1 use it
    #define BAROMETER    1 //
    #define TEMPERATURE  1 //
    #define HUMIDITY     0 //

//NEED TO BE MORE DEV IF WE WILL USE IT
#define USE_BMI     0 //Same as USE_BMX
    #define ACC     0
    #define GYRO    0


#define USE_SI1133  0 //TODO Integrate with Pulga

/*
    FAILSAFE PARAMETERS
*/

// The usage of wdt to recover the sysmte if it faisl
#define USE_WDT     0 //WATCHDOG USAGE
    #define WDT_TIMEOUT     10*WAKEUP_PERIOD

#define MAX_TIME_WTHT_SYNC  10





/*
    CONSTANTS
*/

#define MSEC_PER_SEC   (1000)
#define USEC_PER_SEC    1000*MSEC_PER_SEC
#define TICK_RTC_NS 30517


/*
    BLE MARMONET
*/

#define KEY_SIZE        0x2 //ONE FOR THE FIELD ONE FOR THE NAME
#define KEY             0xCA
#define KEY_FAIL_SAFE   0xFF

/*
    BLE
*/

#define CALLITHRIX_SVC_UUID                 BLE_UUID128_DECLARE(0x02, 0xb4, 0x54, 0xb7, 0xc1, 0x9f, 0x4d, 0x1c, 0xa2, 0xc0, 0xb7, 0xfc, 0x10, 0xf8, 0xa8, 0xa5)

#define CALLITHRIX_CHR_SYNC_TIMER_UUID      BLE_UUID128_DECLARE(0x68, 0x2d, 0x75, 0xcf, 0x44, 0xfc, 0x4d, 0xf4, 0xac, 0x81, 0x00, 0xf0, 0x2a, 0xa9, 0xb9, 0x8c)
#define CALLITHRIX_CHR_SYNC_LAT_UUID        BLE_UUID128_DECLARE(0x9b, 0x86, 0x06, 0xb1, 0x41, 0x17, 0x4a, 0xea, 0x94, 0xdc, 0xa3, 0x95, 0x6a, 0xc8, 0xcd, 0xe5)

#define CALLITHRIX_CHR_DATA_TRANSFER        BLE_UUID128_DECLARE(0xd6, 0xa1, 0x1e, 0x21, 0xc6, 0xe1, 0x49, 0xed, 0x83, 0x55, 0x83, 0x9e, 0x6f, 0xe3, 0x15, 0x1e)

#define CALLITRHIX_CHR_SENSOR_ON            BLE_UUID128_DECLARE(0xC0, 0x66, 0xFF, 0x11, 0x76, 0x88, 0x41, 0x43, 0x9A, 0x01, 0x79, 0x6B, 0xA6, 0xE6, 0x65, 0x57)
#define CALLITRHIX_CHR_SENSOR_OFF           BLE_UUID128_DECLARE(0x12, 0x0c, 0x19, 0x4e, 0xc8, 0x10, 0x4a, 0x5e, 0xa1, 0x31, 0x54, 0x39, 0xb0, 0x13, 0x91, 0x06)


#define CALLITHRIX_CHR_STATUS_UUID          BLE_UUID128_DECLARE(0x59, 0xDE, 0x87, 0xC8, 0x9D, 0x62, 0x4D, 0x18, 0x88, 0x4E, 0xE5, 0x51, 0xB8, 0x38, 0x5F, 0x85)

#define MAXPAYLOAD_SIZE     BLE_ATT_MTU_MAX //bytes


#endif
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

#define LONG_SLEEP 1*WAKEUP_PERIOD


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
    #define USE_BAROMETER    1 //
    #define USE_TEMPERATURE  1 //
    #define USE_HUMIDITY     1 //

//NEED TO BE MORE DEV IF WE WILL USE IT
#define USE_BMI     (0) //Same as USE_BMX
    #define USE_ACC     (0)
    #define USE_GYRO    (0)


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

#define CALLITHRIX_SVC_UUID                 BT_UUID_128_ENCODE(0x02b454b7, 0xc19f, 0x4d1c, 0xa2c0, 0xb7fc10f8a8a5)

#define CALLITHRIX_CHR_SYNC_TIMER_UUID      BT_UUID_128_ENCODE(0x682d75cf, 0x44fc, 0x4df4, 0xac81, 0x00f02aa9b98c)
#define CALLITHRIX_CHR_SYNC_LAT_UUID        BT_UUID_128_ENCODE(0x9b8606b1, 0x4117, 0x4aea, 0x94dc, 0xa3956ac8cde5)

#define CALLITHRIX_CHR_DATA_TRANSFER        BT_UUID_128_ENCODE(0xd6a11e21, 0xc6e1, 0x49ed, 0x8355, 0x839e6fe3151e)

#define CALLITRHIX_CHR_SENSOR_MASK            BT_UUID_128_ENCODE(0xC066FF11, 0x7688, 0x4143, 0x9A01, 0x796BA6E66557)
// #define CALLITRHIX_CHR_SENSOR_OFF           BT_UUID_128_ENCODE(0x120c194e, 0xc810, 0x4a5e, 0xa131, 0x5439b0139106)


#define CALLITHRIX_CHR_STATUS_UUID          BT_UUID_128_ENCODE(0x59DE87C8, 0x9D62, 0x4D18, 0x884E, 0xE551B8385F85)

// #define MAXPAYLOAD_SIZE     BLE_ATT_MTU_MAX //bytes


#endif
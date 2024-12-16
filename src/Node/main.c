#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"
#include "shell.h"
#include "ztimer.h"

#include "../ble_interface.h"
#include "node_global.h"
#include "../bmx_simple.h"

/* advertising data struct */
bluetil_ad_t ad_std;
bluetil_ad_t ad_fail;

//
event_queue_t _eq;
event_t _update_evt;
event_timeout_t _update_timeout_evt;

uint8_t encounters;
uint8_t encounters_fails;

bool con;
uint16_t _conn_handle;

uint32_t timer;
uint8_t last_sync;
bool fail_safe; //REMOVE TO EMBED IN THE FSM


MarmoNet_CallithrixData Data;

//GATT API
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Battery Level Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = CALLITHRIX_SVC_UUID, 

        .characteristics = (struct ble_gatt_chr_def[]) 
        { 
            {
                .uuid = CALLITHRIX_CHR_SYNC_TIMER_UUID,
                .access_cb = gatt_write_timer_handler,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = CALLITHRIX_CHR_SYNC_LAT_UUID,
                .access_cb = gatt_lat_read_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = CALLITHRIX_CHR_DATA_TRANSFER,
                .access_cb = gatt_data_transfer_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = CALLITRHIX_CHR_SENSOR_ON,
                .access_cb = gatt_sensor_set_handler,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = CALLITHRIX_CHR_STATUS_UUID,
                .access_cb = gatt_status_read_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                0, /* no more characteristics in this service */
            }, 
        }
    },
    {
        0, /* no more services */
    },
};


void update_data()
{
    last_sync++;
    
    #if USE_FAIL_SAFE
        fail_safe = last_sync > MAX_TIME_WTHT_SYNC ? true : false; 
    #endif
    //TODO which is more optimal always running this code or the if?
    // if(encounters != my_id || encounters_fails != 0 || (current_mask & MARMONET_MASK_ID) != 0){

    if(Data.info.current_mask == 0) return 0; //All sensors deativated
        //Increment the stack to be send

        //Preparing stack head
        MarmoNet_NodeWakeup* wakeup =  malloc(sizeof(MarmoNet_NodeWakeup));


        //copying
        memcpy(&((*wakeup).event.neighbors_id), &encounters, sizeof(encounters));
        //I think it is more optimal to do a simple attribution, since it is just a uint8_t
        memset(&encounters, Data.info.my_id, sizeof(encounters));

        memcpy(&((*wakeup).event.fail_safe_found), &encounters_fails, sizeof(encounters_fails));
        memset(&encounters_fails, 0, sizeof(encounters_fails));

        //I think it is possible to optimize it using global pointers and a single fucntion call
#if USE_BMX
            (*wakeup).event.enviroment.barometer = Data.info.current_mask & MARMONET_MASK_PRESSURE ? get_pressure() : 0;
            //TODO FIX humidity (not a priority)
            (*wakeup).event.enviroment.humidity = Data.info.current_mask & MARMONET_MASK_HUMIDITY ? get_humidity() : 0;
            (*wakeup).event.enviroment.temperature = Data.info.current_mask & MARMONET_MASK_TEMPERATURE ? get_temperature() : 0;
#endif
        (*wakeup).event.event_n = Data.info.n_wakeup;
        (*wakeup).stack_wakeup = Data.stack_head_wakeup;
        Data.stack_head_wakeup = wakeup;
        Data.info.not_sent_wakeup++;


    // }

}



void _adv_routine()
{
    nimble_scanner_stop();
    adv_start(ad_std, TURN_DURATION);
    //Not tested in the demo it was just a ztimer_sleep(TURN)
    while(ble_gap_adv_active()){ztimer_sleep(ZTIMER_MSEC, 100); LED0_TOGGLE;} //TODO is there a better way to do it
    LED0_ON;
    if(!con){
        nimble_autoadv_stop();
        nimble_scanner_start();
    }
}

void wakeup_callback(void *arg)
{
    /*
        Turn the LED ON to have visual feedback for the demo
    */

    LED0_TOGGLE;

    //Makesure no connection is held and avoid kernel panic

    //FSM to chose which routine to run
#if USE_FAIL_SAFE
    switch (fail_safe)
    {
        //Fail safe routine (straightfoward)
        case true:
                // Setting timer for the nextwakeup
                event_timeout_set(&_update_timeout_evt, WAKEUP_FAIL_SAFE);
                
                if (_conn_handle != BLE_HS_CONN_HANDLE_NONE && con)
                    disconnect_from_central(_conn_handle);
                
                ztimer_sleep(ZTIMER_MSEC,100); //Need to wait the disconnection, otherwise kernel panic
                adv_start(ad_fail, FAIL_SAFE_ADV_TIMER);
                // ztimer_sleep(ZTIMER_MSEC, FAIL_SAFE_ADV_TIMER);
            break;
    
        //Std routine
        default:
            // Setting timer for the nextwakeup
            event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);
            
            if (_conn_handle != BLE_HS_CONN_HANDLE_NONE && con)
                disconnect_from_central(_conn_handle);
            
            ztimer_sleep(ZTIMER_MSEC,100); //Need to wait the disconnection, otherwise kernel panic
            //STARTING WKP ROUTINE
            Data.info.n_wakeup++;
            
            if(Data.info.current_mask & MARMONET_MASK_ID)
            {
                //START SCANNING FOR NONSYNCED SYSTEMS
                
                nimble_scanner_start();
                // ztimer_sleep(ZTIMER_MSEC, TOLERANCE_TIME_ZONE);
            
                /*
                    ROUNDROBIN ROUTINE
                */
                for(uint8_t turn = 0; turn < MAX_NODES; turn++){
                    if(my_id == (1<<turn)) _adv_routine();
                    ztimer_sleep(ZTIMER_MSEC, TURN_DURATION);
                }

                //SCANNING FOR NONSYNCED SYSTESMS  
                // ztimer_sleep(ZTIMER_MSEC, TOLERANCE_TIME_ZONE);
                if(!con) nimble_scanner_stop();
            }else
            {
                adv_start(ad_std, TURN_DURATION);
            }
            /*
                UPDATE THE DATA COLLECTED FROM MARMONET
            */
            update_data();
            printf("CON HANDLE: %i", _conn_handle);
            printf("\r\nLast data: %li, %i", Data.stack_head_wakeup->event.event_n,Data.stack_head_wakeup->event.neighbors_id);
            printf("\r\nbefore data: %li, %i", Data.stack_head_wakeup->stack_wakeup->event.event_n,Data.stack_head_wakeup->stack_wakeup->event.neighbors_id);
            printf("\r\nnot sent: %li", Data.info.not_sent_wakeup);            
            break;
    }
#else
    Data.info.n_wakeup++;
    if(Data.info.n_wakeup%(NMBR_WKPS_SEQ) == 0) event_timeout_set(&_update_timeout_evt, LONG_SLEEP); 
    else event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);
    
    if (_conn_handle != BLE_HS_CONN_HANDLE_NONE && con)
        disconnect_from_central(_conn_handle);
    
    ztimer_sleep(ZTIMER_MSEC,100); //Need to wait the disconnection, otherwise kernel panic
    //STARTING WKP ROUTINE
    
    if(Data.info.current_mask & MARMONET_MASK_ID)
    {
        
        nimble_scanner_start();
        // ztimer_sleep(ZTIMER_MSEC, TOLERANCE_TIME_ZONE);
    
        /*
            ROUNDROBIN ROUTINE
        */
        for(uint8_t turn = 0; turn < MAX_NODES; turn++){
            if(Data.info.my_id == (1<<turn)) _adv_routine();
            else ztimer_sleep(ZTIMER_MSEC, TURN_DURATION);
        }

        //SCANNING FOR NONSYNCED SYSTESMS  
        // ztimer_sleep(ZTIMER_MSEC, TOLERANCE_TIME_ZONE);
        if(!con) nimble_scanner_stop();
    }else
    {
        adv_start(ad_std, TURN_DURATION);
    }

    /*
        UPDATE THE DATA COLLECTED FROM MARMONET
    */
    update_data();

    print_event(Data.stack_head_wakeup->event);
 
#endif

    LED0_TOGGLE;
}

int main(void)
{
    Data.info.my_id = MARMONET_ID_COLOMBINI;
    Data.info.current_mask = MARMONET_MASK_ID | MARMONET_MASK_PRESSURE | MARMONET_MASK_TEMPERATURE | MARMONET_MASK_HUMIDITY;

    uint8_t key = KEY;
    uint8_t key_fails_safe = KEY_FAIL_SAFE;

    last_sync = 0;

    encounters = Data.info.my_id;
    encounters_fails = 0;

    uint8_t key_ptr[] = {KEY_SIZE, 0xFF, KEY, 0x02, 0x10};
    uint16_t _conn_handle = BLE_HS_CONN_HANDLE_NONE;

    uint8_t buf_std[BLE_HS_ADV_MAX_SZ];
    uint8_t buf_fail[BLE_HS_ADV_MAX_SZ];
    // (void)puts("Welcome to MARMONET!");


    //INIT BMX
        //use metaprogramming to only compile this part of the code if the difine is true
    #if USE_BMX
        bmx_simple_init();
    #endif
    //INIT BMI TODO
        //use metaprogramming to only compile this part of the code if the difine is true


    //CONFIGURE BLUETOOTH
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    ble_gatts_start();
    // bluetil_ad_init_with_flags(&ad_std, buf_std, sizeof(buf_std), 
    //                             BLUETIL_AD_FLAGS_DEFAULT);
    bluetil_ad_init(&ad_std, buf_std, 0, sizeof(buf_std));

    //my own ble pipeline using the more basic sutils
    bluetil_ad_add(&ad_std, 0xFF, &key, sizeof(key));
    bluetil_ad_add(&ad_std, 0x10, &Data.info.my_id, sizeof(Data.info.my_id));
    
    // bluetil_ad_init(&ad_fail, buf_fail, sizeof(buf_fail),
    //                 BLUETIL_AD_FLAGS_DEFAULT);
    bluetil_ad_init(&ad_fail, buf_fail, 0, sizeof(buf_fail));

    bluetil_ad_add(&ad_fail, 0xFF, &key_fails_safe, sizeof(key_fails_safe));
    bluetil_ad_add(&ad_fail, 0x10, &Data.info.my_id, sizeof(Data.info.my_id));
    
    adv_set(&gap_event_cb);


    nimble_scanner_cfg_t marmonet_scan_params = {
            .itvl_ms = 30,
            .win_ms = 30,
    #if IS_USED(MODULE_NIMBLE_PHY_CODED)
            .flags = NIMBLE_SCANNER_PHY_1M | NIMBLE_SCANNER_PHY_CODED,
    #else
            .flags = NIMBLE_SCANNER_PHY_1M,
    #endif
    };
 
    nimble_scanner_init(&marmonet_scan_params, scan_callback);

    event_queue_init(&_eq);
    _update_evt.handler = wakeup_callback;
    event_timeout_ztimer_init(&_update_timeout_evt, ZTIMER_MSEC, &_eq, &_update_evt);
    event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);

    // puts("\n\rEverything set");

    event_loop(&_eq);
    return 0;
}


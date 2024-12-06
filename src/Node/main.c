#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nimble_riot.h"
#include "nimble_autoadv.h"
#include "nimble_scanner.h"

#include "thread.h"
#include "shell.h"
#include "ztimer.h"


#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "net/ble.h"

#include "../bmx_simple.c"

#include <stdint.h>
#include "event/timeout.h"
#include "sema.h"

#include "../marmonet_helpers.c"




/**
 * Event configurations
*/
static event_queue_t _eq;
static event_t _update_evt;
static event_timeout_t _update_timeout_evt;
/**
 * MarmoNet Configuration
*/

const MarmoNet_ID my_id = MARMONET_ID_LOUISE;
uint8_t key = KEY;
uint8_t key_fails_safe = KEY_FAIL_SAFE;

MarmoNet_CallithrixData Data;

// MarmoNet_NodeWakeup wakeup;

uint8_t encounters = my_id;
uint8_t encounters_fails = 0;

//pattern to read adv
uint8_t key_ptr[] = {KEY_SIZE, 0xFF, KEY, 0x02, 0x10};


bool con;
static uint16_t _conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t _notify_status_handle;


uint32_t timer;
uint8_t last_sync;
bool fail_safe;

/* buffer for _ad */
static uint8_t buf_std[BLE_HS_ADV_MAX_SZ];
static uint8_t buf_fail[BLE_HS_ADV_MAX_SZ];

/* advertising data struct */
static bluetil_ad_t ad_std;

static bluetil_ad_t ad_fail;


/*
    EGAP event CALLBACK to initiate gap procedure
*/
static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if(event->connect.status==0){
                // printf("Connect\n\r");
                con = true;
                _conn_handle = event->connect.conn_handle;
                ble_gap_disc_cancel();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            // printf("DISCONNECT\n\r");
            _conn_handle = BLE_HS_CONN_HANDLE_NONE;
            con = false;
            break;
    }

    return 0;
}


/*
    Pop event from the stack to send
*/
MarmoNet_NodeWakeup* pop_event(){
    
    MarmoNet_NodeWakeup* to_send = Data.stack_head_wakeup;
    Data.stack_head_wakeup = Data.stack_head_wakeup->stack_wakeup;
    return to_send;
}

/*
    Transfer data to BS
*/
static int gatt_data_transfer_cb(uint16_t conn_handle, uint16_t attr_handle, 
                                    struct ble_gatt_access_ctxt *ctxt, void *arg) 
{

    int res = 0;

    // for(int append = 0; Data.not_sent_wakeup > 0 && append < MAX_EVENT_SEND;
    //         Data.not_sent_wakeup--, append++){
        MarmoNet_NodeWakeup* to_send = pop_event();

        res = os_mbuf_append(ctxt->om, &((*to_send).event), sizeof(MarmoNet_Event));

        if(res != 0) return BLE_ATT_ERR_INSUFFICIENT_RES;
        
        free(to_send);
    // }
        
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    
}   


/*
    Write the timer to sync the system
*/
static int gatt_write_timer_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    
    os_mbuf_copydata(ctxt->om, 0, sizeof(timer), &timer);
    event_timeout_set(&_update_timeout_evt, timer);
    last_sync = 0;
    fail_safe = false;

    return 0;
}

/*
    We are using this cb to read estimate the latency.
    We are sending an immediate to reduce the processing time inside the MCU and getting only the latency delay
*/
static int gatt_lat_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
                            struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
        uint8_t custom_data = 1;
        // Respond with immediate
        int rc = os_mbuf_append(ctxt->om, &custom_data, sizeof(custom_data));
        if (rc != 0) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return 0;
}

/*
    Read the status of the data (how many data there are in the queue, etc)
*/
static int gatt_status_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
                            struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
        // Respond with all the data separeted
        // int rc = os_mbuf_append(ctxt->om, &(Data.info.my_id), sizeof(Data.info.my_id));
        // rc = os_mbuf_append(ctxt->om, &(Data.info.not_sent_wakeup), sizeof(Data.info.not_sent_wakeup));
        // rc = os_mbuf_append(ctxt->om, &(Data.info.n_wakeup), sizeof(Data.info.n_wakeup));
        // rc = os_mbuf_append(ctxt->om, &(Data.info.current_mask), sizeof(Data.info.current_mask));
        //respond with all the data in a block
        int rc = os_mbuf_append(ctxt->om, &(Data.info), sizeof(Data.info));
        

        if (rc != 0) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return 0;
}

static int gatt_sensor_set_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t set_mask;
    os_mbuf_copydata(ctxt->om, 0, sizeof(set_mask), &set_mask);

    Data.info.current_mask = set_mask;
    
    return 0;
}



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
        memset(&encounters, my_id, sizeof(encounters));

        memcpy(&((*wakeup).event.fail_safe_found), &encounters_fails, sizeof(encounters_fails));
        memset(&encounters_fails, 0, sizeof(encounters_fails));

        //I think it is possible to optimize it using global pointers and a single fucntion call
#if USE_BMX
            (*wakeup).event.enviroment.barometer = Data.info.current_mask & MARMONET_MASK_PRESSURE ? get_pressure() : 0;
            //TODO FIX humidity (not a priority)
            // (*wakeup).event.enviroment.humidity = current_mask & MARMONET_MASK_HUMIDITY ? get_humidity() : 0;
            (*wakeup).event.enviroment.temperature = Data.info.current_mask & MARMONET_MASK_TEMPERATURE ? get_temperature() : 0;
#endif
        (*wakeup).event.event_n = Data.info.n_wakeup;
        (*wakeup).stack_wakeup = Data.stack_head_wakeup;
        Data.stack_head_wakeup = wakeup;
        Data.info.not_sent_wakeup++;


    // }

}


void scan_callback(uint8_t type, const ble_addr_t *addr,
                    const nimble_scanner_info_t *info,
                    const uint8_t *ad, size_t len)
{
    //TODO write it better
    // if(memcmp(ad, {}) ==  0){}
    if(*(ad) == KEY_SIZE && *(ad+1) == 0xFF && *(ad+2) == KEY && *(ad+3) == 0x02 && *(ad+4) == 0x10){
        // printf("DATA %i\r\n", *(ad+5));
        encounters = encounters | ((*(ad+5)));
    }
    else if (*(ad) == KEY_SIZE && *(ad+1) == 0xFF && *(ad+2) == KEY_FAIL_SAFE && *(ad+3) == 0x02 && *(ad+4) == 0x10)
    {
        encounters_fails = encounters_fails | ((*(ad+5)));
    }
    
}


void _adv_routine()
{
    nimble_scanner_stop();
    adv_start(ad_std, TURN_DURATION);
    //Not tested in the demo it was just a ztimer_sleep(TURN)
    while(ble_gap_adv_active()){ztimer_sleep(ZTIMER_MSEC, TURN_DURATION);} //TODO is there a better way to do it
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

    // LED0_TOGGLE;

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
        //START SCANNING FOR NONSYNCED SYSTEMS
        
        nimble_scanner_start();
        // ztimer_sleep(ZTIMER_MSEC, TOLERANCE_TIME_ZONE);
    
        /*
            ROUNDROBIN ROUTINE
        */
        for(uint8_t turn = 0; turn < MAX_NODES; turn++){
            if(my_id == (1<<turn)) _adv_routine();
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
    // printf("CON HANDLE: %i", _conn_handle);
    // printf("\r\nLast data: %li, %i, %i", Data.stack_head_wakeup->event.event_n,Data.stack_head_wakeup->event.neighbors_id, Data.stack_head_wakeup->event.enviroment.barometer);
    // printf("\r\nbefore data: %li, %i, %i", Data.stack_head_wakeup->stack_wakeup->event.event_n,Data.stack_head_wakeup->stack_wakeup->event.enviroment.barometer);
    // printf("\r\nnot sent: %li", Data.info.not_sent_wakeup);   
#endif

    // LED0_TOGGLE;
    // pm_set_lowest();
}

int main(void)
{

    // (void)puts("Welcome to MARMONET!");


    //INIT BMX
        //use metaprogramming to only compile this part of the code if the difine is true
    #if USE_BMX
        bmx_simple_init();
    #endif
    //INIT BMI TODO
        //use metaprogramming to only compile this part of the code if the difine is true

    Data.info.my_id = my_id;
    Data.info.current_mask = MARMONET_MASK_ID | MARMONET_MASK_PRESSURE | MARMONET_MASK_TEMPERATURE;

    //CONFIGURE BLUETOOTH
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    ble_gatts_start();
    bluetil_ad_init_with_flags(&ad_std, buf_std, sizeof(buf_std), 
                                BLUETIL_AD_FLAGS_DEFAULT);
    //my own ble pipeline using the more basic sutils
    bluetil_ad_add(&ad_std, 0xFF, &key, sizeof(key));
    bluetil_ad_add(&ad_std, 0x10, &my_id, sizeof(my_id));
    
    bluetil_ad_init(&ad_fail, buf_fail, sizeof(buf_fail),
                    BLUETIL_AD_FLAGS_DEFAULT);
    bluetil_ad_add(&ad_fail, 0xFF, &key_fails_safe, sizeof(key_fails_safe));
    bluetil_ad_add(&ad_fail, 0x10, &my_id, sizeof(my_id));
    
    adv_set(&gap_event_cb);
 
    nimble_scanner_init(&marmonet_scan_params, scan_callback);

    event_queue_init(&_eq);
    _update_evt.handler = wakeup_callback;
    event_timeout_ztimer_init(&_update_timeout_evt, ZTIMER_MSEC, &_eq, &_update_evt);
    event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);

    // puts("\n\rEverything set");

    event_loop(&_eq);
    return 0;
}


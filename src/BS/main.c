/*
 * Copyright (C) 2019 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       (Mock-up) BLE heart rate sensor example
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Hendrik van Essen <hendrik.ve@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <stdint.h>
#include "shell.h"
#include "shell_commands.h"
#include "timex.h"
#include "ztimer.h"

#include "nimble_scanner.h"
#include "nimble_scanlist.h"
#include "nimble_addr.h"
#include "shell.h"
#include "shell_commands.h"
#include <stdlib.h>     /* strtol */
#include "periph/rtt.h"

#include "../marmonet_structs.h"
#include "event/timeout.h"


/**
 * Event configurations
*/
static event_queue_t _eq;
static event_t _update_evt;
static event_timeout_t _update_timeout_evt;
 


static const struct ble_gap_conn_params ble_gap_conn_params_dflt = {
    .scan_itvl = 0x0010,
    .scan_window = 0x0010,
    .itvl_min = BLE_GAP_INITIAL_CONN_ITVL_MIN,
    .itvl_max = BLE_GAP_INITIAL_CONN_ITVL_MAX,
    .latency = BLE_GAP_INITIAL_CONN_LATENCY,
    .supervision_timeout = BLE_GAP_INITIAL_SUPERVISION_TIMEOUT,
    .min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
    .max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN,
};

int conectado = 0;
int returnCOM = 0;
struct os_mbuf data[30];

static uint16_t conected_handler;


struct ble_gap_event* event_global;



uint8_t pulga_c19[]   = {0xA8, 0x4F, 0xB1, 0x56, 0x0B, 0xF4};
uint8_t pulga_epx01[] = {0x0A, 0x43, 0x3B, 0x72, 0xB5, 0xE4};
uint8_t pulga_V1_71[] = {0x9D, 0x62, 0xD9, 0xE0, 0x59, 0xD4};

// typedef uint8_t ble_addr[8];

// ble_addr acessed[8];

bool c19_check = false;
bool epx01_check = false;
bool V1_71_check = false;


ztimer_now_t c19_time;
ztimer_now_t epx01_time;

uint32_t latency_answer, latency_call;
ztimer_now_t ztimer_latency_answer, ztimer_latency_call;
uint32_t latency;


bool epx01_syncing = false;
bool c19_syncing = false;
bool V1_71_syncing = false;



bool sucessefully_connect = false;
bool sucessefully_Services = false;
bool sucessefully_write = false;


static struct
{
    uint16_t lat_handler;
    uint16_t data_handler;
    uint16_t sync_handler;
    uint16_t status_handler;
    
} chrs_list;

bool syncing;

uint8_t data_to_be_recovered = 0;

// MarmoNet_Event data_from_node;
MarmoNet_BSData bs_data;

uint8_t current_mask = MARMONET_MASK_ID;

int rc[14];

uint32_t recieved_timer;



/***
 * 
 *  BLE GATT TO PHONE
 * 
 */


/*
    Pop event from the stack to send
*/
MarmoNet_NodeWakeup* pop_event(){
    
    MarmoNet_NodeWakeup* to_send = bs_data.stack_head_wakeup;
    bs_data.stack_head_wakeup = bs_data.stack_head_wakeup->stack_wakeup;
    return to_send;
}

/*
    Transfer data to BS
*/
static int gatt_data_transfer_cb(uint16_t conn_handle, uint16_t attr_handle, 
                                    struct ble_gatt_access_ctxt *ctxt, void *arg) 
{

    int res = 0;

    for(int append = 0; bs_data.not_sent_wakeup > 0 && append < MAX_EVENT_SEND; bs_data.not_sent_wakeup--){
        MarmoNet_NodeWakeup* level = pop_event();

        res = os_mbuf_append(ctxt->om, &((*level).event), sizeof(MarmoNet_Event));
        if(res != 0) return BLE_ATT_ERR_INSUFFICIENT_RES;
        
        free(level);
    }
        
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    
}   


/*
    Write the timer to sync the system
*/
static int gatt_write_timer_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint32_t timer;
    
    os_mbuf_copydata(ctxt->om, 0, sizeof(timer), &timer);
    event_timeout_set(&_update_timeout_evt, timer);


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
        uint8_t custom_data = bs_data.not_sent_wakeup;
        // Respond with the custom data
        int rc = os_mbuf_append(ctxt->om, &custom_data, sizeof(custom_data));
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

    current_mask = set_mask;
    
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
            // {
            //     .uuid = CALLITRHIX_CHR_SENSOR_ON,
            //     .access_cb = gatt_sensor_set_handler,
            //     .flags = BLE_GATT_CHR_F_WRITE,
            // },
            {
                0, /* no more characteristics in this service */
            }, 
        }
    },
    {
        0, /* no more services */
    },
};

/**********************
************************
    DATA RECOVERY FUNCTIONS
*************************
************************/


static int ble_central_data_recovery(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0 && attr->om != NULL) 
    {
        // for(uint16_t offset = 0; offset < attr->om->om_len-1; 
        //     offset += sizeof(MarmoNet_Event), data_to_be_recovered--, bs_data.not_sent_wakeup++)
        // {
            MarmoNet_NodeWakeup* recover_data =  malloc(sizeof(MarmoNet_NodeWakeup));
            // if(recover_data == NULL) return 0;
            os_mbuf_copydata(attr->om, 0, sizeof(MarmoNet_Event), &(recover_data->event));

            (*recover_data).stack_wakeup = bs_data.stack_head_wakeup;
            bs_data.stack_head_wakeup = recover_data;
            bs_data.not_sent_wakeup++;
            data_to_be_recovered--;
            printf("\r\nDATTAA %i", recover_data->event.event_n);

        // }
        if(data_to_be_recovered <= 0){ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);}
        else{data_recovery();}          
        // ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }else{
        printf("EROOO %i", error->status );
    }
    return 0;
}


void data_recovery()
{
    int rc = ble_gattc_read(conected_handler, chrs_list.data_handler, ble_central_data_recovery, NULL);

}

/**********************************
 * *******************************
    STATUS READ
 * *****************************
 **********************************/
static int ble_status_reading(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0 && attr->om != NULL) 
    {
        os_mbuf_copydata(attr->om, 0, sizeof(data_to_be_recovered), &data_to_be_recovered);
        printf("data_to_be_recovered: %i\r\n", *(attr->om->om_data));
        data_recovery();
    }
    else printf("EROOO %i", error->status );

    // ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return 0;
}

static void ble_status()
{
    int rc = ble_gattc_read(conected_handler, chrs_list.status_handler, ble_status_reading, NULL);
    if(rc!=0) {
        printf("\r\n ERRO: %i", rc);
    }
}


/**********************************
 * *******************************
 *  SYNC FUNCTIONS
 * *****************************
 **********************************/

uint32_t Cristian_alg(uint32_t timer_1, uint32_t timer_0)
{
    return (timer_1 - timer_0)/2;
}

static int ble_on_write_timer(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg) {
    if (error->status == 0) {
        rc[9] = !c19_syncing || rc[9]? 1 : 0;
        rc[10] = !epx01_syncing || rc[10]? 1 : 0;
        
        printf("sync successful.\n");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        // data_recovery();
    } else {
        printf("Write failed with status: 0x%02X\n", error->status);
    }
    return 0;
}


static int ble_sync(uint32_t lat){

    ztimer_now_t t_sync = 2*WAKEUP_PERIOD - ztimer_now(ZTIMER_MSEC)%(WAKEUP_PERIOD) - lat;
    // printf("SYNC in %lu = %lu - %lu - %lu\r\n", t_sync, 2*WAKEUP_PERIOD, ztimer_now(ZTIMER_MSEC)%(WAKEUP_PERIOD), lat);
    int rc = ble_gattc_write_flat(conected_handler, chrs_list.sync_handler, &t_sync, sizeof(t_sync), ble_on_write_timer, NULL);
    if(rc != 0){
        printf("ERRO DE ESCRITA %i \n\r", rc);
    }

    return 0;
}

static int ble_latency_reading(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr *attr, void *arg)
{
    if(error->status == 0)
    {
        // rc[3] = 666;
        // printf("data %i", *(attr->om->om_data));

        latency_answer = NRF_RTC2->COUNTER;//rtt_get_counter();//ztimer_now(ZTIMER_MSEC);
        
        latency =  Cristian_alg(latency_answer, latency_call)*TICK_RTC_NS;

        ble_sync(latency/1000000);

        // printf("LATENCIA : %lu ns\r\n", Cristian_alg(latency_answer, latency_call)*TICK_RTC_NS);
        // printf("Z_LATENCIA : %lu ns\r\n", Cristian_alg(ztimer_latency_answer, ztimer_latency_call));
    }else{
        printf("EROOO %i", error->status );
    }
    return 0;
}

static void ble_latency(){
        latency_call = NRF_RTC2->COUNTER;//rtt_get_counter();//ztimer_now(ZTIMER_MSEC);
        ztimer_latency_call = ztimer_now(ZTIMER_MSEC);

        int rc = ble_gattc_read(conected_handler, chrs_list.lat_handler, ble_latency_reading, NULL);

        if(rc != 0){
            printf("Error gatt Read: %i", rc);
        }

}



/***
 * 
 * BLE GENERAL FUNCTIONS
 * 
 */

static int gap_event_cb(struct ble_gap_event *event, void *arg);

//TODO REWRITE IT
void scan_callback(uint8_t type, const ble_addr_t *addr,
                    const nimble_scanner_info_t *info,
                    const uint8_t *ad, size_t len)
{


    if(memcmp(addr->val, pulga_epx01, 6) == 0)
    {

        if(epx01_syncing){

            sucessefully_connect = false;
            ble_gap_disc_cancel();
            returnCOM = ble_gap_connect(BLE_OWN_ADDR_RANDOM, addr,
                    BLE_HS_FOREVER, &ble_gap_conn_params_dflt, &gap_event_cb, NULL);
            epx01_syncing = false;
            if(returnCOM != 0)
            {
                printf("Failled to connect %i \r\n", returnCOM);
            }

        }
    }

    if(memcmp(addr->val, pulga_c19, 6) == 0)
    {

        if(c19_syncing){
            sucessefully_connect = false;
            ble_gap_disc_cancel();
            returnCOM = ble_gap_connect(BLE_OWN_ADDR_RANDOM, addr,
                    BLE_HS_FOREVER, &ble_gap_conn_params_dflt, &gap_event_cb, NULL);
            c19_syncing = false;
            if(returnCOM != 0)
            {
                printf("Failled to connect %i \r\n", returnCOM);
            }
            // nimble_scanner_start();

        }
    }

    //     if(memcmp(addr->val, pulga_V1_71, 6) == 0)
    // {
    //     if(!V1_71_check)
    //     {
    //         c19_time = ztimer_now(ZTIMER_MSEC);
    //         V1_71_check = true;    
    //     }
    //     if(V1_71_syncing){
    //         sucessefully_connect = false;
    //         ble_gap_disc_cancel();
    //         returnCOM = ble_gap_connect(BLE_OWN_ADDR_RANDOM, addr,
    //                 BLE_HS_FOREVER, &ble_gap_conn_params_dflt, &gap_event_cb, NULL);
    //         V1_71_syncing = false;
    //         if(returnCOM != 0)
    //         {
    //             printf("Failled to connect %i \r\n", returnCOM);
    //         }
    //         // nimble_scanner_start();

    //     }
    // }



}

//TODO add all the chars here
static int ble_central_on_characteristic_discovered(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg) {

    

    if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_SYNC_LAT_UUID) == 0){

        chrs_list.lat_handler = chr->val_handle;

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_SYNC_TIMER_UUID) == 0){

        chrs_list.sync_handler = chr->val_handle;   

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_DATA_TRANSFER) == 0){
       
        chrs_list.data_handler = chr->val_handle;   

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_STATUS_UUID) == 0){
        chrs_list.status_handler = chr->val_handle;
    }

    return 0;
}




static int ble_central_on_service_discovered(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *svc, void* arg)
{    
    sucessefully_Services = true;
         rc[3] = 222;

    if(ble_uuid_cmp(&(svc->uuid.u), CALLITHRIX_SVC_UUID) == 0){
        NRF_RTC2->TASKS_CLEAR = RTC_INTENCLR_TICK_Msk;

        sucessefully_Services = true;

        rc[6] = ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle, ble_central_on_characteristic_discovered, NULL);
        if (rc[6] != 0) {
            printf("Failed to discover characteristics, error: %d\n", rc[6]);
        }
        }
    return 0;
}

static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        /* code */
        if (event->connect.status == 0) {
            printf("Connected to device\n");
            conected_handler = event->connect.conn_handle;

            ble_gattc_disc_all_svcs(conected_handler, ble_central_on_service_discovered, NULL);

        // Proceed with GATT operations like service discovery or reading characteristics
        } else {
            printf("Failed to connect; status=%d\n", event->connect.status);
        }


        break;
    case BLE_GAP_EVENT_DISCONNECT:
        syncing = false;
        chrs_list.lat_handler = 0;
        chrs_list.sync_handler = 0;
        chrs_list.status_handler = 0;
        printf("disconnect\r\n");
        nimble_scanner_start();
        break;


    case BLE_GAP_EVENT_NOTIFY_RX:
            LED0_TOGGLE;
            recieved_timer = ztimer_now(ZTIMER_MSEC);
            uint32_t buf;
            os_mbuf_copydata(event->notify_rx.om, 0, sizeof(buf), &buf);

            printf("%lu | %u \r\n", recieved_timer, buf);

            break;
    
    default:
        break;
    }
    return 0;

}


/*
    Print the neigborhood in a serial prompt
*/
void print_neighborhood_string(void){
    //prints because buffer sometimes(most of them) do not send the first X informations to the terminal


    printf("---------\n\r");


    printf("Connectado %i \n\r", conectado);
    printf("returnCOM %i \n\r", returnCOM);
    printf("Conectado : %i\r\n", sucessefully_connect);
    printf("services : %i\r\n", sucessefully_Services);
    printf("Write : %i\r\n", sucessefully_write);
    printf("hander : %i\r\n", conected_handler);
    // printf("rc3 : %li\r\n", data_from_node.event_n);
    // printf("rc6 : %i\r\n", data_from_node.neighbors);
    printf("sync c19 : %i\r\n", rc[9]);
    printf("sync epx : %i\r\n", rc[10]);
    printf("rc1 : %i\r\n", rc[1]);
    printf("LATENCIA : %lu us/10", latency);
    printf("\r\nrecover: %u", data_to_be_recovered);
    printf("last timer recivied: %lu\r\n", recieved_timer);
    
    printf("recovered dada : %i ", bs_data.not_sent_wakeup);
    MarmoNet_NodeWakeup* wkp = bs_data.stack_head_wakeup;
    for(;bs_data.not_sent_wakeup > 0; bs_data.not_sent_wakeup--){
        printf("\r\n %i %i %i %i", wkp->event.event_n, wkp->event.neighbors_id,
                                         wkp->event.enviroment.temperature,   wkp->event.enviroment.barometer);
        wkp = (*wkp).stack_wakeup;
    }
    bs_data.stack_head_wakeup = wkp;
    printf("---------\n\r");

}



/***
 * 
 *  ACTIVYT FLOW
 * 
 */

void _cmd_sync(int argc, char **argv)
{
    epx01_syncing = true;
    V1_71_syncing = true;
    c19_syncing = true;
    syncing = false;
    nimble_scanner_set_scan_duration(20*MSEC_PER_SEC);
    nimble_scanner_start();
    uint32_t t0 = ztimer_now(ZTIMER_MSEC);
    while( (ztimer_now(ZTIMER_MSEC) - t0)  < 20*MSEC_PER_SEC){
        if(chrs_list.lat_handler != 0 && chrs_list.sync_handler!=0 && !syncing){
            ble_latency();
            syncing = true;
        }
        // if(!ble_gap_disc_active()) nimble_scanner_start();
        ztimer_sleep(ZTIMER_MSEC, 1);
    }

}

void _cmd_print(int argc, char **argv)
{
    print_neighborhood_string();

}



void _cmd_data(int argc, char **argv)
{
    epx01_syncing = true;
    c19_syncing = true;

    syncing = false;

    nimble_scanner_set_scan_duration(20*MSEC_PER_SEC);
    nimble_scanner_start();
    syncing = false;
    uint32_t t0 = ztimer_now(ZTIMER_MSEC);
    while( (ztimer_now(ZTIMER_MSEC) - t0)  < 20*MSEC_PER_SEC){
        if(chrs_list.status_handler != 0 && chrs_list.lat_handler != 0 && chrs_list.sync_handler!=0 && !syncing && chrs_list.data_handler != 0 && !syncing){
            ble_status();
            syncing = true;
        }
        ztimer_sleep(ZTIMER_MSEC, 1);
    }

}


static const shell_command_t _commands[] = {
    { "sync", "trigger a BLE scan", _cmd_sync },
    { "print", "trigger a BLE scan", _cmd_print },
    { "data", "trigger a BLE scan", _cmd_data },


    { NULL, NULL, NULL }
};

void wakeup_callback(void *arg)
{
    event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);

    epx01_syncing = true;
    V1_71_syncing = true;
    c19_syncing = true;

    syncing = false;
    nimble_scanner_set_scan_duration(20*MSEC_PER_SEC);
    nimble_scanner_start();
    uint32_t t0 = ztimer_now(ZTIMER_MSEC);
    while( (ztimer_now(ZTIMER_MSEC) - t0)  < 20*MSEC_PER_SEC){
        if(chrs_list.lat_handler != 0 && chrs_list.sync_handler!=0 && !syncing){
            ble_status();
            ble_latency();
            syncing = true;
        }
        // if(!ble_gap_disc_active()) nimble_scanner_start();
        ztimer_sleep(ZTIMER_MSEC, 1);
    }
}


int main(void)
{
    puts("NimBLE Central \n\r");



    ztimer_init();
    rtt_init();
    nimble_scanner_init(&marmonet_scan_params, scan_callback);
    NRF_RTC2 -> TASKS_START = 1;


    event_queue_init(&_eq);
    _update_evt.handler = wakeup_callback;
    event_timeout_ztimer_init(&_update_timeout_evt, ZTIMER_MSEC, &_eq, &_update_evt);
    event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);

    // puts("\n\rEverything set");
    bs_data.my_id = 0; //ID to identify whch one is
    bs_data.not_sent_wakeup = 0;

    // printf("EVENT QUEUE\r\n");
    // event_loop(&_eq);

    printf("CMD LINE\r\n");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    // return 0;
}


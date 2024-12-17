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

#include "event/timeout.h"
#include "../bmx_simple.h"

#include "../marmonet_helpers.h"

#include "../ble_interface.h"

#include "bs_global.h"

/**
 * Event configurations
*/
event_queue_t _eq;
event_t _update_evt;
event_timeout_t _update_timeout_evt;
 

uint16_t conected_handler;

const struct ble_gap_conn_params ble_gap_conn_params_dflt = 
{
    .scan_itvl = 0x0010,
    .scan_window = 0x0010,
    .itvl_min = BLE_GAP_INITIAL_CONN_ITVL_MIN,
    .itvl_max = BLE_GAP_INITIAL_CONN_ITVL_MAX,
    .latency = BLE_GAP_INITIAL_CONN_LATENCY,
    .supervision_timeout = BLE_GAP_INITIAL_SUPERVISION_TIMEOUT,
    .min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
    .max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN,
};


const mac_addr node_addr[3] = {
    {0xA8, 0x4F, 0xB1, 0x56, 0x0B, 0xF4},  // pulga_c19
    {0x0A, 0x43, 0x3B, 0x72, 0xB5, 0xE4},  // pulga_epx01
    {0x9D, 0x62, 0xD9, 0xE0, 0x59, 0xD4}   // pulga_V1_71
};



bool epx01_syncing = false;
bool c19_syncing = false;
bool V1_71_syncing = false;


chrs chrs_list;


bool syncing;

// MarmoNet_Event data_from_node;
MarmoNet_BSData Data;

int rc[14];



/***
 * 
 *  BLE GATT TO PHONE
 * 
 */


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
            MarmoNet_NodeWakeup* recover_wkp =  malloc(sizeof(MarmoNet_NodeWakeup));
            // if(recover_data == NULL) return 0;
            os_mbuf_copydata(attr->om, 0, sizeof(MarmoNet_Event), &(recover_wkp->event));

            // (*recover_wkp).stack_wakeup = bs_Data.stack_head_collection->data.stack_head_wakeup;
            // bs_Data.stack_head_collection->data.stack_head_wakeup = recover_wkp;
            // bs_data.info.not_sent_wakeup++;
            Data.stack_head_collection->data.stack_size++;
            printf("\r\nstack_size %i", Data.stack_head_collection->data.stack_size);
            print_event(recover_wkp->event);
            // printf("\r\nDATTAA %i", recover_data->event.event_n);

        // }
        if(Data.stack_head_collection->data.stack_size < Data.stack_head_collection->data.abi_info.not_sent_wakeup)
            {data_recovery();}
        else {ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);}
        // else{data_recovery();}          
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
        os_mbuf_copydata(attr->om, 0, sizeof(Data.stack_head_collection->data.abi_info), &(Data.stack_head_collection->data.abi_info));
        // data_to_be_recovered = bs_Data.stack_head_collection->data.abi_info.not_sent_wakeup;
        printf("data_to_be_recovered: %i\r\n", Data.stack_head_collection->data.abi_info.not_sent_wakeup);
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


static int ble_sync(uint32_t lat)
{
    // sync_data _sync = {
    //     .timer = 2*WAKEUP_PERIOD - ztimer_now(ZTIMER_MSEC)%(WAKEUP_PERIOD) - lat,
    //     .current_wakeup = Data.info.n_wakeup
    // };
    uint32_t _sync = 2*WAKEUP_PERIOD - ztimer_now(ZTIMER_MSEC)%(WAKEUP_PERIOD) - lat;
    // printf("SYNC in %lu = %lu - %lu - %lu\r\n", t_sync, 2*WAKEUP_PERIOD, ztimer_now(ZTIMER_MSEC)%(WAKEUP_PERIOD), lat);
    int rc = ble_gattc_write_flat(conected_handler, chrs_list.sync_handler, &(_sync), sizeof(_sync), ble_on_write_timer, NULL);
    if(rc != 0){
        printf("ERRO DE ESCRITA %i \n\r", rc);
    }

    return 0;
}

static int ble_latency_reading(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr *attr, void *arg)
{
    if(error->status == 0)
    {
        // printf("data %i", *(attr->om->om_data));

        uint32_t latency_answer = NRF_RTC2->COUNTER;
        uint32_t latency_call = (int)(uintptr_t)arg;
        
        uint32_t latency =  Cristian_alg(latency_answer, latency_call)*TICK_RTC_NS;

        ble_sync(latency/1000000);

        // printf("LATENCIA : %lu ns\r\n", Cristian_alg(latency_answer, latency_call)*TICK_RTC_NS);
    }else{
        printf("EROOO %i", error->status );
    }
    return 0;
}

static void ble_latency(){
        uint32_t latency_call = NRF_RTC2->COUNTER;//rtt_get_counter();//ztimer_now(ZTIMER_MSEC);

        int rc = ble_gattc_read(conected_handler, chrs_list.lat_handler, ble_latency_reading, (void*)(uintptr_t)latency_call);

        if(rc != 0){
            printf("Error gatt Read: %i", rc);
        }

}



/***
 * 
 * BLE GENERAL FUNCTIONS
 * 
 */






/*
    Print the neigborhood in a serial prompt
*/
void print_bs_data(void){
    //prints because buffer sometimes(most of them) do not send the first X informations to the terminal


    printf("---------\n\r");

    
    printf("\r\nrecovered dada : %i ", Data.info.not_sent_wakeup);
    MarmoNet_data_recover* recover_data = &(Data.stack_head_collection->data);
    printf("\r\nOrigin of this data: %i ID, %i samples", (*recover_data).abi_info.my_id, (*recover_data).stack_size);
    MarmoNet_NodeWakeup* wkp = (*recover_data).stack_head_wakeup;

    for(uint8_t index = 0; index < (*recover_data).stack_size; index++)
    {
        print_event((*wkp).event);
        wkp = wkp->stack_wakeup;
    }
    MarmoNet_BS_Collection* aux = Data.stack_head_collection;
    Data.stack_head_collection = (*aux).stack_collection;
    free(aux);
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
    nimble_scanner_stop();

}

void _cmd_print(int argc, char **argv)
{
    print_bs_data();

}



void _cmd_data(int argc, char **argv)
{
    MarmoNet_BS_Collection* col =  malloc(sizeof(MarmoNet_BS_Collection));
    (*col).stack_collection = Data.stack_head_collection;
    Data.stack_head_collection = col;
    Data.stack_head_collection->data.stack_size = 0;

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
    nimble_scanner_stop();

}

void set_mask(uint8_t mask)
{
    int rc = ble_gattc_write_flat(conected_handler, chrs_list.maks_handler, &(mask), sizeof(mask), ble_on_write_timer, NULL);
    if(rc != 0){
        printf("ERRO DE ESCRITA %i \n\r", rc);
    }


}

void _cmd_mask(int argc, char **argv)
{
    uint8_t mask_value = strtol(argv[1], NULL, 10);

    epx01_syncing = true;
    c19_syncing = true;

    syncing = false;

    nimble_scanner_set_scan_duration(20*MSEC_PER_SEC);
    nimble_scanner_start();
    syncing = false;
    uint32_t t0 = ztimer_now(ZTIMER_MSEC);
    while( (ztimer_now(ZTIMER_MSEC) - t0)  < 20*MSEC_PER_SEC){
        if(chrs_list.status_handler != 0 && chrs_list.lat_handler != 0 && chrs_list.sync_handler!=0 && !syncing && chrs_list.data_handler != 0 && !syncing){
            set_mask(mask_value);
            syncing = true;
        }
        ztimer_sleep(ZTIMER_MSEC, 1);
    }
    nimble_scanner_stop();

}



static const shell_command_t _commands[] = {
    { "sync", "trigger a BLE scan", _cmd_sync },
    { "print", "trigger a BLE scan", _cmd_print },
    { "data", "trigger a BLE scan", _cmd_data },
    { "mask", "set a new MASK", _cmd_mask},


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
    NRF_RTC2 -> TASKS_START = 1;


    event_queue_init(&_eq);
    _update_evt.handler = wakeup_callback;
    event_timeout_ztimer_init(&_update_timeout_evt, ZTIMER_MSEC, &_eq, &_update_evt);
    event_timeout_set(&_update_timeout_evt, WAKEUP_PERIOD);

    // puts("\n\rEverything set");
    Data.info.my_id = 0; //ID to identify whch one is
    Data.info.not_sent_wakeup = 0;
    Data.info.current_mask = MARMONET_MASK_ID;

    // printf("EVENT QUEUE\r\n");
    // event_loop(&_eq);

    printf("CMD LINE\r\n");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    // return 0;
}


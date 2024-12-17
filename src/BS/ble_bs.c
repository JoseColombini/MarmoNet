#include "bs_global.h"

#include "../ble_interface.h"

/*
    EGAP event CALLBACK to initiate gap procedure
*/

//TODO add all the chars here
static int _ble_central_on_characteristic_discovered(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg) {

    

    if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_SYNC_LAT_UUID) == 0){

        chrs_list.lat_handler = chr->val_handle;

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_SYNC_TIMER_UUID) == 0){

        chrs_list.sync_handler = chr->val_handle;   

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_DATA_TRANSFER) == 0){
       
        chrs_list.data_handler = chr->val_handle;   

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITHRIX_CHR_STATUS_UUID) == 0){
        chrs_list.status_handler = chr->val_handle;

    }else if(ble_uuid_cmp(&(chr->uuid.u), CALLITRHIX_CHR_SENSOR_ON) == 0){
        chrs_list.maks_handler = chr->val_handle;
    }

    return 0;
}

static int _ble_central_on_service_discovered(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *svc, void* arg)
{    

    if(ble_uuid_cmp(&(svc->uuid.u), CALLITHRIX_SVC_UUID) == 0){
        NRF_RTC2->TASKS_CLEAR = RTC_INTENCLR_TICK_Msk;


        int rc = ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle, _ble_central_on_characteristic_discovered, NULL);
        if (rc != 0) {
            printf("Failed to discover characteristics, error: %d\n", rc);
        }
        }
    return 0;
}


int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        /* code */
        if (event->connect.status == 0) {
            puts("Connected to device\n");
            conected_handler = event->connect.conn_handle;
            ble_gattc_disc_all_svcs(conected_handler, _ble_central_on_service_discovered, NULL);

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
        conected_handler = BLE_HS_CONN_HANDLE_NONE;
        nimble_scanner_start();
        break;


    case BLE_GAP_EVENT_NOTIFY_RX:

            break;
    
    default:
        break;
    }
    return 0;

}


/*
    Pop event from the stack to send
*/
static MarmoNet_NodeWakeup* pop_event()
{
    
    // MarmoNet_NodeWakeup* to_send = Data.stack_head_collection;
    // Data.stack_head_collection = Data.stack_head_collection->stack_collection;
    // return to_send;
}

/*
    Transfer data to BS
*/
int gatt_data_transfer_cb(uint16_t conn_handle, uint16_t attr_handle, 
                                    struct ble_gatt_access_ctxt *ctxt, void *arg) 
{

    // int res = 0;

    // // for(int append = 0; Data.not_sent_wakeup > 0 && append < MAX_EVENT_SEND;
    // //         Data.not_sent_wakeup--, append++){
    //     MarmoNet_NodeWakeup* to_send = pop_event();

    //     res = os_mbuf_append(ctxt->om, &((*to_send).event), sizeof(MarmoNet_Event));

    //     if(res != 0) return BLE_ATT_ERR_INSUFFICIENT_RES;
        
    //     free(to_send);
    // // }
        
    // return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    
}   


/*
    Write the timer to sync the system
*/
int gatt_write_timer_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    
    // os_mbuf_copydata(ctxt->om, 0, sizeof(timer), &timer);
    // event_timeout_set(&_update_timeout_evt, timer);
    
    // last_sync = 0;
    // fail_safe = false;

    // return 0;
}

/*
    We are using this cb to read estimate the latency.
    We are sending an immediate to reduce the processing time inside the MCU and getting only the latency delay
*/
int gatt_lat_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
                            struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
        // uint8_t custom_data = 1;
        // // Respond with immediate
        // int rc = os_mbuf_append(ctxt->om, &custom_data, sizeof(custom_data));
        // if (rc != 0) {
        //     return BLE_ATT_ERR_INSUFFICIENT_RES;
        // }
        // return 0;
}

/*
    Read the status of the data (how many data there are in the queue, etc)
*/
int gatt_status_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
                            struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
 
        // int rc = os_mbuf_append(ctxt->om, &(Data.info), sizeof(Data.info));
        

        // if (rc != 0) {
        //     return BLE_ATT_ERR_INSUFFICIENT_RES;
        // }
        // return 0;
}

int gatt_sensor_set_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // uint8_t set_mask;
    // os_mbuf_copydata(ctxt->om, 0, sizeof(set_mask), &set_mask);

    // Data.info.current_mask = set_mask;
    
    // return 0;
}



//TODO REWRITE IT
void scan_callback(uint8_t type, const ble_addr_t *addr,
                    const nimble_scanner_info_t *info,
                    const uint8_t *ad, size_t len)
{


    if(memcmp(addr->val, node_addr[0], 6) == 0)
    {

        // if(epx01_syncing){

            ble_gap_disc_cancel();
            int returnCOM = ble_gap_connect(BLE_OWN_ADDR_RANDOM, addr,
                    BLE_HS_FOREVER, &ble_gap_conn_params_dflt, &gap_event_cb, NULL);
            // epx01_syncing = false;
            if(returnCOM != 0)
            {
                printf("Failled to connect %i \r\n", returnCOM);
            }

        // }
    }

    if(memcmp(addr->val, node_addr[1], 6) == 0)
    {

        // if(c19_syncing){
            ble_gap_disc_cancel();
            int returnCOM = ble_gap_connect(BLE_OWN_ADDR_RANDOM, addr,
                    BLE_HS_FOREVER, &ble_gap_conn_params_dflt, &gap_event_cb, NULL);
            // c19_syncing = false;
            if(returnCOM != 0)
            {
                printf("Failled to connect %i \r\n", returnCOM);
            }

        // }
    }

}




#include "node_global.h"

#include "../ble_interface.h"

/*
    EGAP event CALLBACK to initiate gap procedure
*/
int gap_event_cb(struct ble_gap_event *event, void *arg)
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
static MarmoNet_NodeWakeup* pop_event()
{
    
    MarmoNet_NodeWakeup* to_send = Data.stack_head_wakeup;
    Data.stack_head_wakeup = Data.stack_head_wakeup->stack_wakeup;
    return to_send;
}

/*
    Transfer data to BS
*/
int gatt_data_transfer_cb(uint16_t conn_handle, uint16_t attr_handle, 
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
int gatt_write_timer_handler(uint16_t conn_handle, uint16_t attr_handle,
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
int gatt_lat_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
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
int gatt_status_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
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

int gatt_sensor_set_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t set_mask;
    os_mbuf_copydata(ctxt->om, 0, sizeof(set_mask), &set_mask);

    Data.info.current_mask = set_mask;
    
    return 0;
}



void scan_callback(uint8_t type, const ble_addr_t *addr,
                    const nimble_scanner_info_t *info,
                    const uint8_t *ad, size_t len)
{
    //TODO write it better

    if(*(ad) == KEY_SIZE && *(ad+1) == 0xFF && *(ad+2) == KEY && *(ad+3) == 0x02 && *(ad+4) == 0x10){
        // printf("DATA %i\r\n", *(ad+5));
        encounters = encounters | ((*(ad+5)));
    }
    else if (*(ad) == KEY_SIZE && *(ad+1) == 0xFF && *(ad+2) == KEY_FAIL_SAFE && *(ad+3) == 0x02 && *(ad+4) == 0x10)
    {
        encounters_fails = encounters_fails | ((*(ad+5)));
    }
    
}



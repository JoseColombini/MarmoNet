#ifndef _BLE_INTERFACE
#define _BLE_INTERFACE

#include "marmonet_structs.h"
#include "MarmoNet_params.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble_riot.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "net/bluetil/ad.h"

#include <stdlib.h>

int gap_event_cb(struct ble_gap_event *event, void *arg);


int gatt_data_transfer_cb(uint16_t conn_handle, uint16_t attr_handle, 
                                    struct ble_gatt_access_ctxt *ctxt, void *arg);

int gatt_write_timer_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg);


int gatt_lat_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
                            struct ble_gatt_access_ctxt *ctxt, void *arg);


int gatt_status_read_cb(uint16_t conn_handle, uint16_t attr_handle, 
                            struct ble_gatt_access_ctxt *ctxt, void *arg);

int gatt_sensor_set_handler(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg);

void scan_callback(uint8_t type, const ble_addr_t *addr,
                    const nimble_scanner_info_t *info,
                    const uint8_t *ad, size_t len);



#endif
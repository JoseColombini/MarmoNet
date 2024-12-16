#ifndef _MARMONET_HELPERS
#define _MARMONET_HELPERS

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

// Function to disconnect from the central
void disconnect_from_central(uint16_t conn_handle);



uint8_t marmonet_activate_sensor(DATA_MASK sensor);


static ble_gap_event_fn *_gap_cb;
static void *_gap_cb_arg;
static struct ble_gap_adv_params _advp;

void adv_set(ble_gap_event_fn *_gap_event_adv_cb);


void adv_start(bluetil_ad_t _ad, int32_t _adv_duration);


void print_event(MarmoNet_Event event);


#endif
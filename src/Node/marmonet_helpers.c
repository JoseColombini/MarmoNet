#include "../marmonet_helpers.h"

// Function to disconnect from the central
void disconnect_from_central(uint16_t conn_handle) {
    int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (rc == 0) {
        printf("Disconnection initiated successfully.\n");
    } else {
        printf("Failed to initiate disconnection; error code: %d\n", rc);
    }
}



uint8_t marmonet_activate_sensor(DATA_MASK sensor)
{
    return 1;
}






static ble_gap_event_fn *_gap_cb;
static void *_gap_cb_arg;
static struct ble_gap_adv_params _advp;

void adv_set(ble_gap_event_fn *_gap_event_adv_cb)
{
    _gap_cb = _gap_event_adv_cb;
    _gap_cb_arg = NULL;


    memset(&_advp, 0, sizeof _advp);
    _advp.conn_mode = BLE_GAP_CONN_MODE_UND;
    _advp.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // int rc = 0;
    // (void) rc;

    // if (IS_ACTIVE(NIMBLE_AUTOADV_FLAG_FIELD) && BLUETIL_AD_FLAGS_DEFAULT != 0) {
    //     rc = bluetil_ad_init_with_flags(&_ad, buf, sizeof(buf),
    //                                     BLUETIL_AD_FLAGS_DEFAULT);
    //     assert(rc == BLUETIL_AD_OK);
    // }
    // else {
    //     bluetil_ad_init(&_ad, buf, 0, sizeof(buf));
    // }

    // if (NIMBLE_AUTOADV_DEVICE_NAME != NULL) {
    //     rc = bluetil_ad_add_name(&_ad, NIMBLE_AUTOADV_DEVICE_NAME);
    //     assert(rc == BLUETIL_AD_OK);
    // }

    // if (ble_gap_adv_active()) {
    //     nimble_autoadv_start();
    // }
}

void adv_start(bluetil_ad_t _ad, int32_t _adv_duration)
{
    int rc;
    (void) rc;

    rc = ble_gap_adv_stop();
    assert(rc == BLE_HS_EALREADY || rc == 0);

    rc = ble_gap_adv_set_data(_ad.buf, _ad.pos);
    assert(rc == 0);

    rc = ble_gap_adv_start(nimble_riot_own_addr_type, NULL, _adv_duration, &_advp, _gap_cb, _gap_cb_arg);
    assert(rc == 0);
}

void print_event(MarmoNet_Event event)
{
    printf("\r\nEvent nmb: %i", event.event_n);
    printf("\r\nProximity: %i", event.neighbors_id);
    printf("\r\nBMX: %i, %i, %i", event.enviroment.barometer, event.enviroment.humidity, event.enviroment.temperature);
    
}
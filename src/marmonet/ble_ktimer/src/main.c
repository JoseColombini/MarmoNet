
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>


#include "marmonet_structs.h"
#include "marmonet_params.h"
#include "marmonet_helpers.h"


#include <zephyr/drivers/gpio.h>
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
#define THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(ble_thread_stack, THREAD_STACK_SIZE);
static struct k_thread ble_thread;
k_tid_t ble_thread_id;

//work
static struct k_work work_adv_start;


#define KEY 0xCA
#define MAX_DEVICES 8

static const struct gpio_dt_spec led_o = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

struct k_timer wakeup_timer;

int err;

const uint8_t my_id = MARMONET_ID_COLOMBINI;

MarmoNet_CallithrixData data;

uint8_t encounters = 0;

//TODO pass it as constant as in riot
static uint8_t std_adv_data[] = { KEY, my_id};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, std_adv_data, sizeof(std_adv_data)),
};


static struct bt_conn *default_conn;


/**
 * @section BLE GATT
*/
/*
    READ LAT & SYNC
*/
// read latency
static ssize_t gatt_read_lat(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const uint8_t value = 0x00;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

//Write the sync time to the nextwake up
static ssize_t gatt_write_sync(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, uint16_t len, uint16_t offset)
{
    k_timer_start(&wakeup_timer, K_MSEC(*((uint32_t*)buf)),  K_MSEC(WAKEUP_PERIOD));

    return len;
}

/*
    READ MASK & SET NEW MASK
*/

//READ MASK
static ssize_t gatt_read_mask(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
    printk("mask reading %i\n", data.info.current_mask);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &(data.info.current_mask),
                            sizeof(data.info.current_mask));
}

//SET NEW MASK
ssize_t gatt_write_new_mask(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	
    memcpy(&(data.info.current_mask), buf, len);

	return len;
}

/*
    READ AND RECOVER DATA
*/

static ssize_t gatt_read_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
    MarmoNet_NodeWakeup* wakeup;
    wakeup = data.stack_head_wakeup;
    data.stack_head_wakeup = data.stack_head_wakeup->stack_wakeup;
    if(wakeup == NULL) return 0;

    ssize_t ret = bt_gatt_attr_read(conn, attr, buf, len, offset, wakeup,
                            sizeof(*wakeup));
    free(wakeup);

    return ret;
}

/*
    READ NODE INFO
*/
static ssize_t gatt_read_inf(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &(data.info),
                            sizeof(data.info));
}


/*
    GATT APIs
*/
static const struct bt_uuid_128 call_svc = BT_UUID_INIT_128(CALLITHRIX_SVC_UUID);
static const struct bt_uuid_128 call_char_sync_lat_uuid = BT_UUID_INIT_128(CALLITHRIX_CHR_SYNC_LAT_UUID);
static const struct bt_uuid_128 call_char_data_uuid = BT_UUID_INIT_128(CALLITHRIX_CHR_DATA_TRANSFER);
static const struct bt_uuid_128 call_char_mask_uuid = BT_UUID_INIT_128(CALLITRHIX_CHR_SENSOR_MASK);
static const struct bt_uuid_128 call_char_info_uuid = BT_UUID_INIT_128(CALLITHRIX_CHR_STATUS_UUID);


/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(marmonet_svc,
	BT_GATT_PRIMARY_SERVICE(&call_svc),
        //Char to catch lat using Cristian's algorithm.
        //It send only a byte to avoid miss reading the latency beacuse of processing time
	    BT_GATT_CHARACTERISTIC(&call_char_sync_lat_uuid.uuid,
		    	                BT_GATT_CHRC_READ,
			                    BT_GATT_PERM_READ,
			                    gatt_read_lat, gatt_write_sync, NULL),
	    //Char to send the data saved in the node
        BT_GATT_CHARACTERISTIC(&call_char_data_uuid.uuid,
		    	                BT_GATT_CHRC_READ,
			                    BT_GATT_PERM_READ,
			                    gatt_read_data, NULL, NULL),
	    //Used to set a mask in the node
        BT_GATT_CHARACTERISTIC(&call_char_mask_uuid.uuid,
		    	                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			                    gatt_read_mask, gatt_write_new_mask, NULL),
	    //Recover all the status of the node
        BT_GATT_CHARACTERISTIC(&call_char_info_uuid.uuid,
		    	                BT_GATT_CHRC_READ,
			                    BT_GATT_PERM_READ,
			                    gatt_read_inf, NULL, NULL),
);


/**
 * @section BLE ROUTINES
*/

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		// bt_conn_unref(default_conn);
		default_conn = NULL;

		k_work_submit(&work_adv_start);

        return;
	}
    // default_conn = bt_conn_ref(conn);
	printk("Connected: %s\n", addr);




	// bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	// if (conn != default_conn) {
	// 	return;
	// }

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	// bt_conn_unref(default_conn);
	default_conn = NULL;

    // k_sleep(K_SECONDS(1));  // Give time for cleanup

    k_work_submit(&work_adv_start);


}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};



static void scan_callback(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
            struct net_buf_simple *buf)
{
    //Detect if the advertise has the key and save it in the encounters variable
    if(buf->data[2] == KEY){
        LOG_INF("Device found: KEY %d ID %d \n", buf->data[2], buf->data[3]);
        encounters |= buf->data[3];
    }
    
}

//Adv routine to do the TDMA of our network
void adv_routine()
{
    //Start the adv
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
                            NULL, 0);

    if(err) LOG_ERR("Bluetooth adv error %i", err);

    //Wait the turn duration
    k_sleep(K_MSEC(TURN_DURATION));

    //Stop the advertise, it will automatically start the scan
    err = bt_le_adv_stop();
    if(err) LOG_ERR("Bluetooth adv stop error %i", err);
}


/**
 * @section DATA HANDLING
*/

// #define USE_BMX 1 to use the BME280 sensor
static void read_sensors(MarmoNet_NodeWakeup* wakeup)
{
#if USE_BMX
    err = fetch_bme280();

    if(!err)
    {
        (*wakeup).event.enviroment.comp_press = data.info.current_mask & MARMONET_MASK_PRESSURE ? get_pressure() : 0;
        (*wakeup).event.enviroment.comp_humidity = data.info.current_mask & MARMONET_MASK_HUMIDITY ? get_humidity() : 0;
        (*wakeup).event.enviroment.comp_temp = data.info.current_mask & MARMONET_MASK_TEMPERATURE ? get_temperature() : 0;
    }
#endif

}

//update the data available and manage the stack memory, also handling with the masks
static void update_data()
{

    
    #if USE_FAIL_SAFE
        fail_safe = last_sync > MAX_TIME_WTHT_SYNC ? true : false; 
    #endif
    //TODO which is more optimal always running this code or the if?
    // if(encounters != my_id || encounters_fails != 0 || (current_mask & MARMONET_MASK_ID) != 0){

    if(data.info.current_mask == 0) return; //All sensors deativated
        //Increment the stack to be send

    //Preparing stack head
    MarmoNet_NodeWakeup* wakeup =  malloc(sizeof(MarmoNet_NodeWakeup));


    //copying
    memcpy(&((*wakeup).event.neighbors_id), &encounters, sizeof(encounters));
    //I think it is more optimal to do a simple attribution, since it is just a uint8_t
    memset(&encounters, data.info.my_id, sizeof(encounters));

    // memcpy(&((*wakeup).event.fail_safe_found), &encounters_fails, sizeof(encounters_fails));
    // memset(&encounters_fails, 0, sizeof(encounters_fails));

    read_sensors(wakeup);

        //I think it is possible to optimize it using global pointers and a single fucntion call

    (*wakeup).event.event_n = data.info.n_wakeup;
    (*wakeup).stack_wakeup = data.stack_head_wakeup;
    data.stack_head_wakeup = wakeup; 
    
    data.info.n_wakeup++;
    data.info.not_sent_wakeup++;
    data.info.last_sync++;
}

/**
 * @section THREADS
*/

//The wakeup thread function will define how the system will work after each wakeup
void wakeup_thread_function(void *arg1, void *arg2, void *arg3)
{

    err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_callback);
	
    if (err) LOG_ERR("Scanning failed to start (err %d)\n", err);
    
    k_sleep(K_MSEC(TURN_DURATION));

    gpio_pin_toggle_dt(&led_o);


    for(int round = 0; round < MAX_DEVICES; round++){
        if((1 << round) == my_id) adv_routine();
        else {k_sleep(K_MSEC(TURN_DURATION));}
    }
    
    err = bt_le_scan_stop();
    
    if(err) LOG_ERR("Bluetooth stop scan error %i", err);

    gpio_pin_toggle_dt(&led_o);

    update_data();

}

//Wakeup callback is the interuption that will start the wakeup thread to start the activities of our system
void wakeup_callback(struct k_timer *timer)
{

    k_thread_create(&ble_thread, ble_thread_stack,
                    K_THREAD_STACK_SIZEOF(ble_thread_stack),
                    wakeup_thread_function, NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    
}


static void adv_start(struct k_work *work)
{
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
                        NULL, 0);
}

/**
 * @section MAIN
*/

//Configure the system to start working
int main() {

	if (!gpio_is_ready_dt(&led_o)) {
		return 0;
	}
    init_bme280();

	gpio_pin_configure_dt(&led_o, GPIO_OUTPUT_ACTIVE);
    

    data.info.my_id = my_id;
    data.info.current_mask = MARMONET_MASK_ID;
    data.info.last_sync = 0;
    data.info.n_wakeup = 0;
    data.info.not_sent_wakeup = 0;

    err = bt_enable(NULL);
    if(err)
        LOG_ERR("Bluetooth Error %i", err);
    
    k_work_init(&work_adv_start, adv_start);
	k_work_submit(&work_adv_start);

    // k_timer_init(&wakeup_timer, wakeup_callback, NULL);

    // k_timer_start(&wakeup_timer, K_MSEC(1000),  K_MSEC(WAKEUP_PERIOD));

}
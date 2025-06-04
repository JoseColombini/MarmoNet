
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
#include <zephyr/drivers/counter.h>


#define LED0_NODE DT_ALIAS(led0)
// #define COUNTER_NODE DT_NODELABEL(rtc2)

#define BT_RX_PRIO_STACK_SIZE 1024


/**
 * TODO
 * Test Env notify
 * Elaborate better the data recover struct using byte array
 * Refactor - break it down in multiple files, incraese readability
 * See the ideia of using indicator vs notify
 * Read notify from the node to recover data fast
 * Increase MTU
 * Put all data recovery in the same UUID (node and environment)
 * Solve the error of **No SOURCES given to Zephyr library: drivers__counter** when compiling
 *      Solution is my old computer, config in dts of pulga the rtc
 *      Uncommect the counter_dev and all realted things
 */

/**
 * @file This file hold the BS node of the Marmonet project.
 *     
*/

/**
 * @section DEFINES and globals
 */
//TODO Pass to the params file if it is possible
#define THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(ble_thread_stack, THREAD_STACK_SIZE);
static struct k_thread ble_thread;
k_tid_t ble_thread_id;

K_THREAD_STACK_DEFINE(wakeup_thread_stack, THREAD_STACK_SIZE);
static struct k_thread wakeup_thread;
k_tid_t wakeup_thread_id;

#define KEY 0xCA
#define MAX_DEVICES 8

static const struct gpio_dt_spec led_o = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
// const struct device *counter_dev = DEVICE_DT_GET(COUNTER_NODE);
uint32_t start_ticks;
uint32_t freq;

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

struct k_timer wakeup_timer;

int err;

const uint8_t my_id = 1;

MarmoNet_BSData BS_data;

uint8_t encounters = 0;

uint8_t events_recovered = 0;

//TODO pass it as constant as in riot
static uint8_t std_adv_data[] = { BS_KEY, my_id};

K_SEM_DEFINE(my_sem, 0, 1); //SEMPAHOR TO DEBUG


static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, std_adv_data, sizeof(std_adv_data)),
};

//Connection
static struct bt_conn *default_conn;
static struct bt_gatt_discover_params discover_service_params;
static struct bt_gatt_discover_params discover_char_params;
static struct bt_gatt_read_params read_params;
static struct bt_gatt_write_params write_params;
static struct k_work work_discover;


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

}

//SET NEW MASK
ssize_t gatt_write_new_mask(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	
}

/*
    READ AND RECOVER DATA
*/

static ssize_t gatt_read_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{

}

/*
    READ NODE INFO
*/
static ssize_t gatt_read_inf(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, uint16_t len, uint16_t offset)
{

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
		    	                BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
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
 * @section Discover and Callbacks
*/


static void gatt_write_cb(struct bt_conn *conn, uint8_t err,
                          struct bt_gatt_write_params *params)
{
    if (err) {
        LOG_ERR("GATT write failed (err %d)\n", err);
    } else {
        LOG_INF("GATT write successful!\n");
    }
    // bt_gatt_read(conn, &read_params);
}

static uint8_t gatt_read_data_cb(struct bt_conn *conn, uint8_t err,
                                struct bt_gatt_read_params *params,
                                const void *data, uint16_t length)
{
    if (err) {
        LOG_ERR("GATT read data fail (err %d)", err);
        return BT_GATT_ITER_STOP;
    }
    if(data){
        // BS_data.data_recovered[BS_data.info.not_sent_wakeup].events[events_recovered] = *(MarmoNet_Event*)data;
        memcpy(&BS_data.data_recovered[BS_data.info.not_sent_recovered].events[events_recovered], data, sizeof(MarmoNet_Event));
        LOG_INF("recover : %i yet to be recovered %i", events_recovered,  BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.not_sent_wakeup);
        events_recovered++;
        if(events_recovered < BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.not_sent_wakeup)
            {int err = bt_gatt_read(default_conn, &read_params);}
        else
            k_sem_give(&my_sem);



    }
    return BT_GATT_ITER_STOP;

}


uint32_t Cristian_alg(uint32_t timer_1, uint32_t timer_0)
{
    return (timer_1 - timer_0)/2;
}



static uint8_t gatt_read_sync_lat_cb(struct bt_conn *conn, uint8_t err,
                                struct bt_gatt_read_params *params,
                                const void *data, uint16_t length)
{

    uint32_t now_ticks;
    // counter_get_value(counter_dev, &now_ticks);
    /*
        TODO The timer is done in level o milliseconds, but we have 30 micro seconds precision
        The problem is that we are not sure about the tick compensation used
        in k_timer and the RTT of our system.

        Future works should explore how is timed this different functions and
        make it a better implementation.
    */
    //Cristian Algorithm
    uint32_t latency_ticks = ((now_ticks - start_ticks)/2) & 0x00FFFFFF;
    uint32_t latency_ms = latency_ticks*TICK_RTC_NS/1000000;
    LOG_INF("LATENCIA %i", latency_ms);

    //TODO in the real system 2* is not needed 
    //the timers are too far away and no conncetion doesnt exist before a call, only after
    //The 2* is used to avoid calling things from the past :)

    uint32_t _sync;
    
    write_params.handle = params->single.handle;
    write_params.data = &_sync;      // Pointer to the data to send
    write_params.length = sizeof(_sync); // Data size
    write_params.func = gatt_write_cb; // Callback for write confirmation

    _sync =  k_timer_remaining_get(&wakeup_timer) - latency_ms + 3000;

    err = bt_gatt_write(conn, &write_params);
    LOG_DBG("Writing new mask: %i", _sync);


}



static uint8_t gatt_read_info_cb(struct bt_conn *conn, uint8_t err,
                                struct bt_gatt_read_params *params,
                                const void *data, uint16_t length)
{
    if (err) {
        LOG_ERR("GATT read info failed (err %d)", err);
        return BT_GATT_ITER_STOP;
    }
    if(data){

        memcpy(&BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info, data, sizeof(MarmoNet_NodeInfo));
        if(BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.current_mask != BS_data.info.current_mask){

            write_params.handle = params->single.handle;
            write_params.data = &BS_data.info.current_mask;      // Pointer to the data to send
            write_params.length = sizeof(BS_data.info.current_mask); // Data size
            write_params.func = gatt_write_cb; // Callback for write confirmation
            LOG_DBG("Writing new mask");


            err = bt_gatt_write(conn, &write_params);
       }
       if(BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.not_sent_wakeup > 0){


            BS_data.data_recovered[BS_data.info.not_sent_recovered].array_size = BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.not_sent_wakeup;
            BS_data.data_recovered[BS_data.info.not_sent_recovered].events = malloc(BS_data.data_recovered[BS_data.info.not_sent_recovered].array_size * sizeof(MarmoNet_Event));
            
            read_params.func = gatt_read_data_cb;
            read_params.by_uuid.uuid = &call_char_data_uuid;
            read_params.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
            read_params.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

            int err = bt_gatt_read(default_conn, &read_params);

       }
    }
    // k_sem_give(&my_sem);
    
    return BT_GATT_ITER_STOP;

}

static uint8_t gatt_read_maks_cb(struct bt_conn *conn, uint8_t err,
                                struct bt_gatt_read_params *params,
                                const void *data, uint16_t length) {
    if (err) {
        LOG_ERR("GATT read failed (err %d)", err);
        return BT_GATT_ITER_STOP;
    }
    if (data) {
        uint8_t status = *(uint8_t *)data; // Assuming data is an integer
        LOG_INF("Received status: %i", status);
    }
    if (data) {
        LOG_INF("Data length: %d", length);
        for (int i = 0; i < length; i++) {
            LOG_INF("Byte %d: 0x%02x", i, ((uint8_t *)data)[i]);
        }
    }
    uint8_t value = 4;

    write_params.handle = params->single.handle;
    write_params.data = &value;      // Pointer to the data to send
    write_params.length = sizeof(value); // Data size
    write_params.func = gatt_write_cb; // Callback for write confirmation


    err = bt_gatt_write(conn, &write_params);
    if (err) {
        LOG_ERR("Failed to write (err %d)\n", err);
    }

    return BT_GATT_ITER_STOP;
}



// static uint8_t gatt_service_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
//                                 struct bt_gatt_discover_params *params) {
//     LOG_DBG("gatt service discover");
//     if (!attr) {
//         LOG_DBG("Discovery completed");
//         return BT_GATT_ITER_STOP;
//     }

//     LOG_DBG("Discovery ongoing");


//     struct bt_gatt_service_val *service = (struct bt_gatt_service_val *)attr->user_data;


//     discover_char_params.uuid = NULL;
//     discover_char_params.func = gatt_char_discover_cb;
//     discover_char_params.start_handle = attr->handle + 1;
//     discover_char_params.end_handle = service->end_handle;
//     discover_char_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

//     bt_gatt_discover(default_conn, &discover_char_params);
//     return BT_GATT_ITER_STOP;
// }

static void work_discover_cb(void *arg1, void *arg2, void *arg3)
{

    BS_data.info.n_wakeup++;



    read_params.func = gatt_read_sync_lat_cb;
    read_params.by_uuid.uuid = &call_char_sync_lat_uuid;
    read_params.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    read_params.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    //Restart the counter before the read to make it more precise
    // counter_get_value(counter_dev, &start_ticks);
    
    int err = bt_gatt_read(default_conn, &read_params);

    //TODO descomentar
    // read_params.func = gatt_read_info_cb;
    // read_params.by_uuid.uuid = &call_char_info_uuid;
    // read_params.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    // read_params.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    // int err = bt_gatt_read(default_conn, &read_params);

    // k_sem_take(&my_sem, K_FOREVER);


    // bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);



    // LOG_INF("INFO READ: \n id: %i mask: %i data to recover: %i", 
    //         BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.my_id,
    //         BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.current_mask,
    //         BS_data.data_recovered[BS_data.info.not_sent_recovered].abi_info.not_sent_wakeup);

    // for(int i = 0; i < events_recovered; i++){
    //         LOG_INF("%i: \r\n"
    //                 "neighbors: %i \r\n"
    //                 "sensors %i %i %i \r\n ", 
    //                 i, 
    //                 BS_data.data_recovered[BS_data.info.not_sent_recovered].events[i].neighbors_id,
    //                 BS_data.data_recovered[BS_data.info.not_sent_recovered].events[i].enviroment.comp_press,
    //                 BS_data.data_recovered[BS_data.info.not_sent_recovered].events[i].enviroment.comp_humidity,
    //                 BS_data.data_recovered[BS_data.info.not_sent_recovered].events[i].enviroment.comp_temp);
    // }    
    
}



/**
 * @section BLE ROUTINES
*/



static void scan_callback(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
            struct net_buf_simple *buf)
{
    //Detect if the advertise has the key and save it in the encounters variable
    if(buf->data[2] == KEY){
        LOG_INF("Device found: KEY %d ID %d ", buf->data[2], buf->data[3]);
        
        err = bt_le_scan_stop();
        
        if(err) LOG_ERR("Bluetooth stop scan error %i", err);

        err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
            BT_LE_CONN_PARAM_DEFAULT, &default_conn);

        if(err) LOG_ERR("Connection error %i", err);

    }
    
}




static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s %u %s", addr, err, bt_hci_err_to_str(err));

		bt_conn_unref(default_conn);
		default_conn = NULL;

		// start_scan();
		return;
	}
    default_conn = bt_conn_ref(conn);
	LOG_INF("Connected: %s", addr);


    // k_work_submit(&work_discover);
    k_thread_create(&ble_thread, ble_thread_stack,
                K_THREAD_STACK_SIZEOF(ble_thread_stack),
                work_discover_cb, NULL, NULL, NULL,
                K_PRIO_COOP(7), 0, K_NO_WAIT);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(default_conn);
	default_conn = NULL;

    gpio_pin_toggle_dt(&led_o);


    BS_data.data_recovered[BS_data.info.not_sent_recovered].array_size = events_recovered;
    events_recovered = 0;
    BS_data.info.not_sent_recovered++;

    

}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};


//Adv routine to do the TDMA of our network
void adv_routine()
{
    //Start the adv
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
                            NULL, 0);

    if(err) LOG_ERR("Bluetooth adv error %i", err);

    //Wait the turn duration
    k_sleep(K_MSEC(BS_UPDATE_DURATION));

    //Stop the advertise, it will automatically start the scan
    err = bt_le_adv_stop();
    if(err) LOG_ERR("Bluetooth adv stop error %i", err);
}




/**
 * @section DATA HANDLING
*/
void update_data()
{
    //SEPARAR ISSO PARA OUTRA FUNCAO EM OUTROS LOCAIS
#if USE_BMX
    err = fetch_bme280();

    if(!err)
    {
        BS_data.bs_enviroment[BS_data.info.not_sent_env].enviroment.comp_press = BS_data.info.current_mask & MARMONET_MASK_PRESSURE ? get_pressure() : 0;
        BS_data.bs_enviroment[BS_data.info.not_sent_env].enviroment.comp_humidity = BS_data.info.current_mask & MARMONET_MASK_HUMIDITY ? get_humidity() : 0;
        BS_data.bs_enviroment[BS_data.info.not_sent_env].enviroment.comp_temp = BS_data.info.current_mask & MARMONET_MASK_TEMPERATURE ? get_temperature() : 0;
        BS_data.bs_enviroment[BS_data.info.not_sent_env].event_n = BS_data.info.n_wakeup;
    }
#endif
    BS_data.info.last_sync++;
    BS_data.info.not_sent_env++;

}


/**
 * @section THREADS
*/

//The wakeup thread function will define how the system will work after each wakeup
void wakeup_thread_function(void *arg1, void *arg2, void *arg3)
{

    err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_callback);
	
    if (err) LOG_ERR("Scanning failed to start (err %d)", err);
    
    k_sleep(K_MSEC(TURN_DURATION*(MAX_DEVICES + 4)));


    adv_routine();
    

    err = bt_le_scan_stop();
    
    if(err) LOG_ERR("Bluetooth stop scan error %i", err);

    gpio_pin_toggle_dt(&led_o);

    update_data();

}

//Wakeup callback is the interuption that will start the wakeup thread to start the activities of our system
void wakeup_callback(struct k_timer *timer)
{

    k_thread_create(&wakeup_thread, wakeup_thread_stack,
                    K_THREAD_STACK_SIZEOF(wakeup_thread_stack),
                    wakeup_thread_function, NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);
    
}

void blinky_test(void *arg1, void *arg2, void *arg3)
{
    LOG_INF("Blinky test");
    gpio_pin_toggle_dt(&led_o);

}

void blinky_cb(struct k_timer *timer)
{

    LOG_INF("Blinky Callback");
    k_thread_create(&wakeup_thread, wakeup_thread_stack,
                    K_THREAD_STACK_SIZEOF(wakeup_thread_stack),
                    blinky_test, NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);    
}






/**
 * @section MAIN
*/

//Configure the system to start working
int main() {

	if (!gpio_is_ready_dt(&led_o)) {
        LOG_ERR("LED PROBLEM");
		return 0;
	}
    // if (!device_is_ready(counter_dev)) {
    //     LOG_ERR("COUNTER PROBLEM");
    //     return;
    // }

    init_bme280();
    // counter_start(counter_dev);
    // uint32_t freq = counter_get_frequency(counter_dev); // usually 32768 Hz

	gpio_pin_configure_dt(&led_o, GPIO_OUTPUT_ACTIVE);


    BS_data.info.my_id = my_id;
    BS_data.info.current_mask = MARMONET_MASK_ID | MARMONET_MASK_PRESSURE | MARMONET_MASK_TEMPERATURE | MARMONET_MASK_HUMIDITY;
    BS_data.info.last_sync = 0;
    BS_data.info.n_wakeup = 0;
    BS_data.info.not_sent_recovered = 0;
    BS_data.info.not_sent_env = 0;


    err = bt_enable(NULL);
    if(err)
        LOG_ERR("Bluetooth Error %i", err);
    
    // k_timer_init(&wakeup_timer, wakeup_callback, NULL);

    //TODO inicio tem q ter um adiantamento, janela expandida
    // k_timer_start(&wakeup_timer, K_MSEC(1000),  K_MSEC(WAKEUP_PERIOD));

    // err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
    //                     NULL, 0);

    // k_work_init(&work_discover, work_discover_cb);

    k_timer_init(&wakeup_timer, blinky_cb, NULL);
    k_timer_start(&wakeup_timer, K_MSEC(1000),  K_MSEC(3000));

    k_sleep(K_MSEC(2000));


    err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_callback);
}
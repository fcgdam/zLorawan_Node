﻿#define   OTAA
//#define ABP

#include <lorawan/lorawan.h>
#include <zephyr.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <zephyr/random/rand32.h>

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
         "Console device is not ACM CDC UART device");

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay), "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lorawan_node);

/* Customize based on network configuration */
// OTAA
#ifdef OTAA
#define LORAWAN_DEV_EUI         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }    // MSB Format!
#define LORAWAN_JOIN_EUI        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }    // MSB Format!
#define LORAWAN_APP_KEY         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#endif

// ABP
#ifdef ABP
#define LORAWAN_DEV_ADDR        { 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_NWK_SKEY        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_SKEY        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_EUI         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#endif

#define DELAY K_MSEC(10000)

char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd','f','r','o','z','e','p','h','y','r'};

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE DT_ALIAS(led0)
//#define LED1_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

uint32_t    mainled_rate = 150;     

void blink(const struct gpio_dt_spec *led, uint32_t *sleep_ms, uint32_t id) {
    int ret;

    if (!device_is_ready(led->port)) {
        return;
    }

    ret = gpio_pin_configure_dt(led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

    while (1) {
        // if the rate is zero we set the LED OFF
        if ( *sleep_ms == 0) {
            gpio_pin_set_dt(led, 0);
            // and we sleep some time to let the task run
            k_msleep(200);
        } else {
            if ( *sleep_ms == 1) {
                // Led always on
                gpio_pin_set_dt(led, 1);
                // and we sleep some time to let the task run
                k_msleep(200);
            } else {
                // Otherwise we just blink the led at the defined rate
                gpio_pin_toggle_dt(led);
                //printk("Blink %d\n", id);
                k_msleep( *sleep_ms);
            }
        }
    }
}

void blink0(void) {

    blink(&led0, &mainled_rate, 0);
}

// Call back functions:
// Downlink callback
static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,  uint8_t len, const uint8_t *data)
{   
    printk("\nDownlink data received: \n");
    for(int i=0; i < len; i++ )
        printk("%02X ", data[i]);
    
    printk("\n");
    printk("Data size: %d\n" , len );
    printk("Data Port: %d\n" , port );
    printk("RSSI:      %d\n" , (int16_t)rssi );
    printk("SNR:       %d\n" , (int16_t)snr );
    printk("Data pend: %d\n" , data_pending );

    printk("\n\n");

}

// ADR change callback
static void lorwan_datarate_changed(enum lorawan_datarate dr)
{   
    uint8_t unused, max_size;
    
    lorawan_get_payload_sizes(&unused, &max_size); 
    LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

/* Main program.
*/

void main(void)
{
	const struct device *lora_dev;
	struct lorawan_join_config join_cfg;
#ifdef OTAA
	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;
#endif

#ifdef ABP
    uint8_t dev_addr[] = LORAWAN_DEV_ADDR;
    uint8_t nwk_skey[] = LORAWAN_NWK_SKEY;
    uint8_t app_skey[] = LORAWAN_APP_SKEY;
    uint8_t app_eui[]  = LORAWAN_APP_EUI;
#endif
	int ret;

    // Give time connect to USB.
    k_msleep(2500);
    mainled_rate = 500;         // Change led blinking rate to signal main program start.

    printk("Starting up Lora node...\n\n");

	lora_dev = device_get_binding(DEFAULT_RADIO);
	if (!lora_dev) {
        printk("Lora default radio not found.\nExiting program.\n");
		return;
	}

    printk("Starting Lorawan stack...\n");
	ret = lorawan_start();
	if (ret < 0) {
		printk("lorawan_start failed: %d\n\n", ret);
		return;
	}

    // Enable ADR
    lorawan_enable_adr( true );
    
    // Enable callbacks
    struct lorawan_downlink_cb downlink_cb = {
        .port = LW_RECV_PORT_ANY,
        .cb = dl_callback
    };

    lorawan_register_downlink_callback( &downlink_cb );
    lorawan_register_dr_changed_callback( lorwan_datarate_changed );

    uint32_t random = sys_rand32_get();

    uint16_t dev_nonce = random & 0x0000FFFF;

#ifdef OTAA
	join_cfg.mode = LORAWAN_CLASS_A;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
    join_cfg.otaa.dev_nonce = dev_nonce;
#endif

#ifdef ABP
    join_cfg.mode = LORAWAN_ACT_ABP;
    join_cfg.dev_eui = dev_addr;
    join_cfg.abp.dev_addr = dev_addr;
    join_cfg.abp.app_skey = app_skey;
    join_cfg.abp.nwk_skey = nwk_skey;
    join_cfg.abp.app_eui  = app_eui;
#endif


	printk("Joining TTN network over");
#ifdef OTAA
    printk(" OTTA\n\n\n");
#else
    printk(" ABP\n\n\n");
#endif

    // Loop until we connect
    do {
    	ret = lorawan_join(&join_cfg);
    	if (ret < 0) {
	    	printk("lorawan_join_network failed: %d\n\n", ret);
            mainled_rate = 100;             // Flash the led very rapidly to signal failure.
            printk("Sleeping for 10s to try again to join network.\n\n");
            k_sleep(K_MSEC(10000));
            mainled_rate = 0;
	    }
    } while ( ret < 0 );

	printk("Sending data...\n\n");

	while (1) {
		ret = lorawan_send(2, data, sizeof(data), LORAWAN_MSG_CONFIRMED);

		if (ret == -EAGAIN) {
			k_sleep(DELAY);
			continue;
		}

		if (ret < 0) {
			k_sleep(DELAY);
			continue;
		}

		printk("Data sent!\n\n");
		k_sleep(DELAY);
	}
}

void console_init(void) {
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
        return;
    }

    /* Poll if the DTR flag was set, optional */
    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_msleep(250);          // Let other tasks to run if no terminal is connected to USB
    }

    while ( 1 ) {
        k_msleep(200000);
        //printk("Ola mundo!\n");
    }
}


// Task for handling blinking leds.
K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL, PRIORITY, 0, 0);        // Main Green LED

// Task for starting up the USB console
K_THREAD_DEFINE(console_id, STACKSIZE, console_init, NULL, NULL, NULL, PRIORITY-2, 0, 0);

// Lorawan handling task to join the TTN network
//K_THREAD_DEFINE(lorawan_task_id, STACKSIZE, lorawan_task, NULL, NULL, NULL, PRIORITY, 0, 0);

/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define   OTAA
//#define ABP

#include <lorawan/lorawan.h>
#include <zephyr.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

/* Customize based on network configuration */
// OTAA
#ifdef OTAA
#define LORAWAN_DEV_EUI			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }    // LSB Format!
#define LORAWAN_JOIN_EUI		{ 0x70, 0xB3, 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_KEY         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#endif

// ABP
#ifdef ABP
#define LORAWAN_DEV_ADDR        { 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_NWK_SKEY        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_SKEY        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_EUI         { 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00 }
#endif

#define DELAY K_MSEC(10000)

char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd','f','r','o','z','e','p','h','y','r'};

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE DT_ALIAS(led0)
//#define LED1_NODE DT_ALIAS(led1)

struct led {
    const char *gpio_dev_name;
    const char *gpio_pin_name;
    unsigned int gpio_pin;
    unsigned int gpio_flags;
};

uint32_t    mainled_rate = 150;     

void blink(const struct led *led, uint32_t *sleep_ms, uint32_t id) {
    struct device *gpio_dev;
    int cnt = 0;
    int ret;

    gpio_dev = device_get_binding(led->gpio_dev_name);
    if (gpio_dev == NULL) {
        printk("Error: didn't find %s device\n",
        led->gpio_dev_name);
        return;
    }

    ret = gpio_pin_configure(gpio_dev, led->gpio_pin, led->gpio_flags);
    if (ret != 0) {
        printk("Error %d: failed to configure pin %d '%s'\n",
        ret, led->gpio_pin, led->gpio_pin_name);
    return;
    }

    while (1) {
        // if the rate is zero we set the LED OFF
        if ( *sleep_ms == 0) {
            gpio_pin_set(gpio_dev, led->gpio_pin, 0);
            // and we sleep some time to let the task run
            k_msleep(200);
        } else {
            if ( *sleep_ms == 1) {
                // Led always on
                gpio_pin_set(gpio_dev, led->gpio_pin, 1);
                // and we sleep some time to let the task run
                k_msleep(200);
            } else {
                // Otherwise we just blink the led at the defined rate
                gpio_pin_set(gpio_dev, led->gpio_pin, cnt % 2);
                //printk("Blink %d\n", id);
                k_msleep( *sleep_ms);
                cnt++;
            }
        }
    }
}

void blink0(void) {
    const struct led led0 = {
    #if DT_NODE_HAS_STATUS(LED0_NODE, okay)
        .gpio_dev_name = DT_GPIO_LABEL(LED0_NODE, gpios),
        .gpio_pin_name = DT_LABEL(LED0_NODE),
        .gpio_pin = DT_GPIO_PIN(LED0_NODE, gpios),
        .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED0_NODE, gpios),
    #else
        #error "Unsupported board: led0 devicetree alias is not defined"
    #endif
    };

    blink(&led0, &mainled_rate, 0);
}

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
    mainled_rate = 500;

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

#ifdef OTAA
	join_cfg.mode = LORAWAN_CLASS_A;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
#endif

#ifdef ABP
    join_cfg.mode = LORAWAN_ACT_ABP;
    join_cfg.dev_eui = dev_addr;
    join_cfg.abp.dev_addr = dev_addr;
    join_cfg.abp.app_skey = app_skey;
    join_cfg.abp.nwk_skey = nwk_skey;
    join_cfg.abp.app_eui  = app_eui;
#endif


	printk("Joining TTN  network over ");
#ifdef OTAA
    printk(" OTTA\n\n\n");
#else
    printk(" ABP\n\n\n");
#endif
	ret = lorawan_join(&join_cfg);
	if (ret < 0) {
		printk("lorawan_join_network failed: %d\n\n", ret);
		return;
	}

	printk("Sending data...\n\n");

	while (1) {
		ret = lorawan_send(2, data, sizeof(data), LORAWAN_MSG_CONFIRMED);

		if (ret == -EAGAIN) {
			k_sleep(DELAY);
			continue;
		}

		if (ret < 0) {
			return;
		}

		printk("Data sent!\n\n");
		k_sleep(DELAY);
	}
}

void console_init(void) {
    struct device *dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
        return;
    }

    /* Poll if the DTR flag was set, optional */
    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_msleep(250);          // Let other tasks to run if no terminal is connected to USB
    }

    if (strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME) != strlen("CDC_ACM_0") ||
        strncmp(CONFIG_UART_CONSOLE_ON_DEV_NAME, "CDC_ACM_0",   strlen(CONFIG_UART_CONSOLE_ON_DEV_NAME))) {
            printk("Error: Console device name is not USB ACM\n");

        return;
    }

    while ( 1 ) {
        k_msleep(20000);
    }
}


// Task for handling blinking leds.
K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL, PRIORITY, 0, 0);        // Main Green LED

// Task for starting up the USB console
K_THREAD_DEFINE(console_id, STACKSIZE, console_init, NULL, NULL, NULL, PRIORITY-2, 0, 0);

// Lorawan handling task to join the TTN network
//K_THREAD_DEFINE(lorawan_task_id, STACKSIZE, lorawan_task, NULL, NULL, NULL, PRIORITY, 0, 0);


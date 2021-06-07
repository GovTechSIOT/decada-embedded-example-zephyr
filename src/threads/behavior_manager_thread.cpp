#include <logging/log.h>
LOG_MODULE_REGISTER(behavior_manager_thread, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/watchdog.h>
#include "threads.h"

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

#define LED0   DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN0   DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS0 DT_GPIO_FLAGS(LED0_NODE, gpios)
#define LED1   DT_GPIO_LABEL(LED1_NODE, gpios)
#define PIN1   DT_GPIO_PIN(LED1_NODE, gpios)
#define FLAGS1 DT_GPIO_FLAGS(LED1_NODE, gpios)
#define LED2   DT_GPIO_LABEL(LED2_NODE, gpios)
#define PIN2   DT_GPIO_PIN(LED2_NODE, gpios)
#define FLAGS2 DT_GPIO_FLAGS(LED2_NODE, gpios)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_watchdog)
#endif
#ifndef WDT_MAX_WINDOW
#define WDT_MAX_WINDOW 5000U
#endif
#ifdef WDT_NODE
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#else
#define WDT_DEV_NAME ""
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif

void execute_behavior_manager_thread(void)
{
	const int sleep_time_ms = 500;

	/* Init GPIO LEDs */
	const struct device* led0;
	const struct device* led1;
	const struct device* led2;
	int ret;
	led0 = device_get_binding(LED0);
	led1 = device_get_binding(LED1);
	led2 = device_get_binding(LED2);
	if (led0 == NULL || led1 == NULL || led2 == NULL) {
		return;
	}
	bool led_is_on[] = { true, true, true };
	const struct device* led_arr[] = { led0, led1, led2 };
	int pin_arr[] = { PIN0, PIN1, PIN2 };
	int flags_arr[] = { FLAGS0, FLAGS1, FLAGS2 };
	int current_led_id = 0;
	for (int i = 0; i < 3; i++) {
		ret = gpio_pin_configure(led_arr[i], pin_arr[i], GPIO_OUTPUT_ACTIVE | flags_arr[i]);
		if (ret < 0) {
			return;
		}
	}

	/* Initialization of Watchdog components in main.cpp */
	int rc, wdt_channel_id;
	const struct device* wdt;

	wdt = device_get_binding(WDT_DEV_NAME);
	if (!wdt) {
		LOG_ERR("Cannot get WDT device");
		return;
	}
	wdt_timeout_cfg wdt_config = wdt_timeout_cfg();
	wdt_config.flags = WDT_FLAG_RESET_SOC;
	wdt_config.window.min = 0U;
	wdt_config.window.max = WDT_MAX_WINDOW;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id < 0) {
		LOG_ERR("Watchdog install error %d", wdt_channel_id);
		return;
	}
	rc = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (rc < 0) {
		LOG_ERR("Watchdog setup error %d", rc);
		return;
	}
	else {
		LOG_INF("Watchdog successfully setup");
	}

	while (true) {
		/* Thread Logic */
		gpio_pin_set(led_arr[current_led_id], pin_arr[current_led_id], (int)led_is_on[current_led_id]);
		led_is_on[current_led_id] = !led_is_on[current_led_id];
		current_led_id = (current_led_id + 1) % 3;

		wdt_feed(wdt, wdt_channel_id);
		k_msleep(sleep_time_ms);
	}
}
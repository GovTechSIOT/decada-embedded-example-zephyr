#include <logging/log.h>
LOG_MODULE_REGISTER(behavior_manager_thread, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/watchdog.h>
#include "conversions/conversions.h"
#include "threads.h"
#include "time_engine/time_engine.h"
#include "watchdog_config/watchdog_config.h"

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

K_MBOX_DEFINE(data_mailbox);

void execute_behavior_manager_thread(int watchdog_id)
{
	const int sleep_time_ms = 5000;
	const struct device* wdt = watchdog_config::get_device_instance();
	const int wdt_channel_id = watchdog_id;

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

	TimeEngine pseudo_sensor;
	std::string sensor_data;
	const int mail_size = 128;

	while (true) {
		/* Thread Logic */
		gpio_pin_set(led_arr[current_led_id], pin_arr[current_led_id], (int)led_is_on[current_led_id]);
		led_is_on[current_led_id] = !led_is_on[current_led_id];
		current_led_id = (current_led_id + 1) % 3;

		/* Read pseudosensor data */
		sensor_data = pseudo_sensor.get_timestamp_s_str();
		LOG_DBG("sensor_data: %s", sensor_data.c_str());

		/* Populate Mailbox and send data to CommunicationsThread*/
		struct k_mbox_msg send_msg;
		send_msg.info = sensor_data.length();
		send_msg.size = sensor_data.length();
		send_msg.tx_data = StringToChar(sensor_data);
		send_msg.tx_target_thread = K_ANY;
		k_mbox_async_put(&data_mailbox, &send_msg, NULL);

		struct k_mbox_msg recv_msg;
		char mailbox_buf[mail_size];
		recv_msg.info = mail_size;
		recv_msg.size = mail_size;
		recv_msg.rx_source_thread = K_ANY;
		k_mbox_get(&data_mailbox, &recv_msg, mailbox_buf, K_NO_WAIT);
		char* d = static_cast<char*>(recv_msg.tx_data);
		size_t len = *static_cast<char*>(recv_msg.tx_data);
		std::string data(d, len);
		free(recv_msg.tx_data);
		if (recv_msg.size != recv_msg.info) {
			LOG_WRN("Mail data corrupted during transfer");
			LOG_INF("Expected size: %d, actual size %d", recv_msg.info, recv_msg.size);
		}
		LOG_DBG("received from mail: %s", data.c_str());

		wdt_feed(wdt, wdt_channel_id);
		k_msleep(sleep_time_ms);
	}
}
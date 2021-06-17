#include <logging/log.h>
LOG_MODULE_REGISTER(behavior_manager_thread, LOG_LEVEL_DBG);

#include "ArduinoJson.hpp"
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/watchdog.h>
#include "conversions/conversions.h"
#include "device_uuid/device_uuid.h"
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

void execute_behavior_manager_thread(int watchdog_id)
{
	const int sleep_time_ms = 10 * MSEC_PER_SEC;
	const struct device* wdt = watchdog_config::get_device_instance();
	const int wdt_channel_id = watchdog_id;

	const std::string decada_protocol_version = "1.0";
	const std::string decada_method_of_device = "thing.measurepoint.post";

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
	bool led_is_on[] = { false, false, false };
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

	/* Wait for DECADA connection to be up before continuing */
	k_poll(decada_connect_ok_events, 1, K_FOREVER);

	while (true) {
		/* Moving LEDs example*/
		gpio_pin_set(led_arr[current_led_id], pin_arr[current_led_id], (int)led_is_on[current_led_id]);
		led_is_on[current_led_id] = !led_is_on[current_led_id];
		current_led_id = (current_led_id + 1) % 3;

		/* Use timestamp as a dummy sensor reading */
		sensor_data = pseudo_sensor.get_timestamp_s_str();
		LOG_DBG("sensor_data: %s", sensor_data.c_str());

		/* Format data into DECADA-compliant JSON */
		ArduinoJson::DynamicJsonDocument params(64);
		params["measurepoints"]["chronos_s"] = sensor_data;

		ArduinoJson::DynamicJsonDocument json(512);
		json["id"] = device_uuid;
		json["version"] = decada_protocol_version;
		json["params"] = params;
		json["method"] = decada_method_of_device;

		std::string json_body;
		ArduinoJson::serializeJson(json, json_body);

		/* Data is placed on the heap */
		int buf_len = json_body.size();
		char* buf = (char*)malloc(buf_len);
		memcpy(buf, json_body.c_str(), buf_len);

		/* Populate Mailbox and send data to CommunicationsThread */
		struct k_mbox_msg send_msg;
		send_msg.info = buf_len;
		send_msg.size = buf_len;
		send_msg.tx_data = buf;
		send_msg.tx_block.data = NULL;
		send_msg.tx_target_thread = K_ANY;
		k_mbox_async_put(&data_mailbox, &send_msg, NULL);

		wdt_feed(wdt, wdt_channel_id);
		k_msleep(sleep_time_ms);
	}
}
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

void execute_behavior_manager_thread(int watchdog_id)
{
	const int sleep_time_ms = 5000;
	const struct device* wdt = watchdog_config::get_device_instance();
	const int wdt_channel_id = watchdog_id;

	TimeEngine pseudo_sensor;
	std::string sensor_data;

	while (true) {
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

		wdt_feed(wdt, wdt_channel_id);
		k_msleep(sleep_time_ms);
	}
}
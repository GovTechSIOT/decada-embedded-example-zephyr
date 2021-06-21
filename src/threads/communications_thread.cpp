#include <logging/log.h>
LOG_MODULE_REGISTER(communications_thread, LOG_LEVEL_DBG);

#include <net/tls_credentials.h>
#include <power/reboot.h>
#include <time.h>
#include "decada_manager/decada_manager.h"
#include "device_uuid/device_uuid.h"
#include "networking/http/http_request.h"
#include "networking/http/http_response.h"
#include "networking/wifi/wifi_connect.h"
#include "persist_store/persist_store.h"
#include "threads.h"
#include "time_engine/time_manager.h"
#include "tls_certs.h"
#include "watchdog_config/watchdog_config.h"

/* Sensor readings topic */
const std::string sensor_pub_topic =
	std::string("/sys/") + USER_CONFIG_DECADA_PRODUCT_KEY + "/" + device_uuid + "/thing/measurepoint/post";
/* DECADA Service topic base */
const std::string decada_service_topic =
	std::string("/sys/") + USER_CONFIG_DECADA_PRODUCT_KEY + "/" + device_uuid + "/thing/service/";
/* DECADA Service - Sensor poll rate */
const std::string sensor_poll_topic = decada_service_topic + "sensorpollrate";

/* Topics to subscribe to */
std::vector<std::string> subscription_topics = { sensor_poll_topic };

struct k_poll_signal decada_connect_ok_signal;
struct k_poll_event decada_connect_ok_events[] = {
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &decada_connect_ok_signal),
};

/**
 *  @brief	Add recognized CA certificates to Zephyr's secure socket layer
 *  @author	Lee Tze Han
 *  @details	This function should only be called once at startup
 */
void add_tls_ca_certs(void)
{
	int rc = tls_credential_add(CA_CERTS_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, CA_CERTS, strlen(CA_CERTS) + 1);
	if (rc < 0) {
		if (rc == -EEXIST) {
			LOG_WRN("CA certificates already exist - add_tls_ca_certs should only be called once");
		}
		else {
			LOG_WRN("Failed to set CA certificates: %d", rc);
		}
	}
}

void execute_communications_thread(int watchdog_id)
{
	const int sleep_time_ms = 250;
	const struct device* wdt = watchdog_config::get_device_instance();
	const int wdt_channel_id = watchdog_id;
	const int rx_buf_size = 512;

	k_poll_signal_init(&wifi_signal);
	k_poll_signal_init(&decada_connect_ok_signal);

	/* Setup WiFi connection */
	wifi_mgmt_event_init();
	wifi_connect();

	/* Block until WiFi connection is established */
	k_poll(wifi_events, 1, K_FOREVER);
	wdt_feed(wdt, wdt_channel_id);

	/* Add recognized CA certificates to secure socket layer */
	add_tls_ca_certs();

	TimeManager time_manager;
	time_manager.sync_sntp_rtc();
	wdt_feed(wdt, wdt_channel_id);

	/* TODO: Requires fix for NVS */
	// init_persist_storage();
	// write_sw_ver("R1.0.0");

	DecadaManager decada_manager(wdt_channel_id);
	wdt_feed(wdt, wdt_channel_id);

	if (!decada_manager.connect()) {
		sys_reboot(SYS_REBOOT_WARM);
	}
	wdt_feed(wdt, wdt_channel_id);

	decada_manager.subscribe(subscription_topics);

	/* Signal other threads that DECADA connection is up */
	k_poll_signal_raise(&decada_connect_ok_signal, 0);

	/* TODO: Requires fix for NVS */
	// std::string sw_ver = read_sw_ver();
	// LOG_DBG("sw_ver (read from flash): %s", sw_ver.c_str());

	while (true) {
		struct k_mbox_msg recv_msg;
		char rx_buf[rx_buf_size];
		recv_msg.info = rx_buf_size;
		recv_msg.size = rx_buf_size;
		recv_msg.rx_source_thread = K_ANY;

		/* Try receiving data from BehaviorManager Thread via Mailbox */
		k_mbox_get(&data_mailbox, &recv_msg, rx_buf, K_FOREVER);
		std::string payload(rx_buf, recv_msg.size);
		free(recv_msg.tx_data);

		if (recv_msg.size != recv_msg.info) {
			LOG_WRN("Mail data corrupted during transfer");
			LOG_INF("Expected size: %d, actual size %d", recv_msg.info, recv_msg.size);
		}
		LOG_DBG("Received from mail: %s", payload.c_str());
		if (!decada_manager.publish(sensor_pub_topic, payload)) {
			sys_reboot(SYS_REBOOT_WARM);
		}

		wdt_feed(wdt, wdt_channel_id);
		k_msleep(sleep_time_ms);
	}
}
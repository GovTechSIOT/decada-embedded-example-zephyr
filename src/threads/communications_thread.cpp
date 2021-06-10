#include <logging/log.h>
LOG_MODULE_REGISTER(communications_thread, LOG_LEVEL_DBG);

#include <net/tls_credentials.h>
#include <time.h>
#include "decada_manager/decada_manager.h"
#include "networking/http/http_request.h"
#include "networking/http/http_response.h"
#include "networking/wifi/wifi_connect.h"
#include "persist_store/persist_store.h"
#include "threads.h"
#include "time_engine/time_manager.h"
#include "tls_certs.h"
#include "watchdog_config/watchdog_config.h"

struct k_poll_signal decada_connect_ok_signal;
struct k_poll_event decada_connect_ok_events[] = {
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &decada_connect_ok_signal),
};

void execute_communications_thread(int watchdog_id)
{
	const int sleep_time_ms = 500;
	const struct device* wdt = watchdog_config::get_device_instance();
	const int wdt_channel_id = watchdog_id;
	const int mail_buf_size = 128;

	k_poll_signal_init(&wifi_signal);
	k_poll_signal_init(&decada_connect_ok_signal);

	/* Setup WiFi connection */
	wifi_mgmt_event_init();
	wifi_connect();

	/* Block until WiFi connection is established */
	k_poll(wifi_events, 1, K_FOREVER);

	/* Set recognized CA certificates */
	int rc = tls_credential_add(CA_CERTS_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, CA_CERTS, strlen(CA_CERTS) + 1);
	if (rc < 0) {
		LOG_WRN("Failed to set CA certificates: %d", rc);
	}

	TimeManager time_manager;
	time_manager.sync_sntp_rtc();

	/* TODO: Requires fix for NVS */
	// init_persist_storage();
	// write_sw_ver("R1.0.0");

	DecadaManager decada_manager;
	decada_manager.connect();

	/* Signal other threads that DECADA connection is up */
	k_poll_signal_raise(&decada_connect_ok_signal, 0);

	/* TODO: Requires fix for NVS */
	// std::string sw_ver = read_sw_ver();
	// LOG_DBG("sw_ver (read from flash): %s", sw_ver.c_str());

	while (true) {
		struct k_mbox_msg recv_msg;
		char mailbox_buf[mail_buf_size];
		recv_msg.info = mail_buf_size;
		recv_msg.size = mail_buf_size;
		recv_msg.rx_source_thread = K_ANY;

		/* Try receiving data from BehaviorManager Thread via Mailbox */
		k_mbox_get(&data_mailbox, &recv_msg, mailbox_buf, K_FOREVER);
		char* d = static_cast<char*>(recv_msg.tx_data);
		std::string data(d, recv_msg.size);
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
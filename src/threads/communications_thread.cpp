#include <logging/log.h>
LOG_MODULE_REGISTER(communications_thread, LOG_LEVEL_DBG);

#include <net/tls_credentials.h>
#include <time.h>
#include "decada_manager/decada_manager.h"
#include "networking/http/http_request.h"
#include "networking/http/http_response.h"
#include "networking/wifi/wifi_connect.h"
#include "threads.h"
#include "time_engine/time_manager.h"
#include "tls_certs.h"

void execute_communications_thread(void)
{
	const int sleep_time_ms = 500;

	k_poll_signal_init(&wifi_signal);

	/* Setup WiFi connection */
	wifi_mgmt_event_init();
	wifi_connect();

	/* Block until WiFi connection is established */
	k_poll(wifi_events, 1, K_FOREVER);

	/* Set recognized CA certificates */
	int rc = tls_credential_add(CA_CERTS_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				    CA_CERTS, strlen(CA_CERTS) + 1);
	if (rc < 0) {
		LOG_WRN("Failed to set CA certificates: %d", rc);
	}

	TimeManager time_manager;
	time_manager.sync_sntp_rtc();

	DecadaManager decada_manager;

	int counter = 0;
	while (true) {
		/* Periodically print Unix epoch timestamp */
		if (counter >= 120) {
			LOG_DBG("Timestamp: %" PRId64,
				time_manager.get_timestamp());
			counter = 0;
		}
		counter++;

		k_msleep(sleep_time_ms);
	}
}
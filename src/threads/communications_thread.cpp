#include <logging/log.h>
LOG_MODULE_REGISTER(communications_thread, LOG_LEVEL_DBG);

#include <time.h>
#include "networking/http/http_request.h"
#include "networking/http/http_response.h"
#include "networking/wifi/wifi_connect.h"
#include "threads.h"
#include "time_engine/time_manager.h"

void execute_communications_thread(void)
{
	const int sleep_time_ms = 500;

	k_poll_signal_init(&wifi_signal);

	/* Setup WiFi connection */
	wifi_mgmt_event_init();
	wifi_connect();

	/* Block until WiFi connection is established */
	k_poll(wifi_events, 1, K_FOREVER);

	TimeManager time_manager;
	time_manager.sync_sntp_rtc();

	HttpRequest http_req("http://qihao.ddns.net", 2001);
	HttpResponse* resp = http_req.send_request(HTTP_POST, "test payload");
	if (resp) {
		LOG_INF("Received HTTP response: %s", resp->get_body().c_str());
	}
	else {
		LOG_ERR("null resp");
	}
	delete resp;

	int counter = 0;
	while (true) {
		/* Periodically print Unix epoch timestamp */
		if (counter >= 10) {
			LOG_DBG("Timestamp: %" PRId64,
				time_manager.get_timestamp());
			counter = 0;
		}
		counter++;

		k_msleep(sleep_time_ms);
	}
}
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_connect, LOG_LEVEL_DBG);

#include "user_config.h"
#include "wifi_connect.h"

static struct net_mgmt_event_callback wifi_mgmt_cb;

struct k_poll_signal wifi_signal;
struct k_poll_event wifi_events[] = {
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &wifi_signal)
};

/**
 * @brief	Callback for connection to WiFi
 * @author	Lee Tze Han
 * @param	cb	Callback structure containing event information
 */
void handle_wifi_connect_result(struct net_mgmt_event_callback* cb)
{
	const struct wifi_status* status = (const struct wifi_status*)cb->info;

	if (status->status) {
		LOG_WRN("Failed to connect to WiFi");
	}
	else {
		k_poll_signal_raise(&wifi_signal, 0);
		LOG_INF("Successfully connected to WiFi");
	}
}

/**
 * @brief	Callback for disconnection to WiFi
 * @author	Lee Tze Han
 * @param	cb	Callback structure containing event information
 * @note	Will also be called when WiFi network is down and connection is dropped
 */
void handle_wifi_disconnect_result(struct net_mgmt_event_callback* cb)
{
	const struct wifi_status* status = (const struct wifi_status*)cb->info;

	if (status->status) {
		LOG_WRN("Error during disconnection from WiFi");
	}
	else {
		LOG_INF("Disconnected from WiFi");
	}
}

/**
 * @brief	Calls the appropriate callback based on the event
 * @author	Lee Tze Han
 * @param	cb			Callback structure containing event information
 * @param	mgmt_event	Event signature
 * @param	netif		Network interface pointer
 */
void wifi_mgmt_event_handler(struct net_mgmt_event_callback* cb, uint32_t mgmt_event, struct net_if* netif)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}
}

/**
 * @brief	Registers events that should call callbacks specified by wifi_mgmt_event_handler
 * @author	Lee Tze Han
 */
void wifi_mgmt_event_init(void)
{
	/* Bitmask of events registered in handler */
	uint32_t registered_wifi_mgmt_events = 
		NET_EVENT_WIFI_CONNECT_RESULT | 
		NET_EVENT_WIFI_DISCONNECT_RESULT;
	
	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler, registered_wifi_mgmt_events);

	net_mgmt_add_event_callback(&wifi_mgmt_cb);
}

/**
 * @brief	Connect to WiFi network asynchronously
 * @author	Lee Tze Han
 * @note	Credentials for connecting to network are specified in src/user_config.h
 */
void wifi_connect(void)
{
	char wifi_ssid[] = USER_CONFIG_WIFI_SSID;
	char wifi_pass[] = USER_CONFIG_WIFI_PASS;

	struct wifi_connect_req_params wifi_params = {
		.ssid = 		(uint8_t*)wifi_ssid,
		.ssid_length = 	(uint8_t)strlen(wifi_ssid),
		.psk = 			(uint8_t*)wifi_pass,
		.psk_length = 	(uint8_t)strlen(wifi_pass),
		.channel = 		WIFI_CHANNEL_ANY,
		.security = 	WIFI_SECURITY_TYPE_PSK
	};

	/* Assumes default interface is WiFi */
	struct net_if* iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(struct wifi_connect_req_params))) {
		LOG_WRN("Failed to request connection to WiFi");
	}
	else {
		LOG_INF("Connecting to WiFi...");
	}
}
#ifndef _WIFI_CONNECT_H_
#define _WIFI_CONNECT_H_

#include <zephyr.h>
#include <net/wifi_mgmt.h>

extern struct k_poll_signal wifi_signal;
extern struct k_poll_event wifi_events[];

void handle_wifi_connect_result(struct net_mgmt_event_callback* cb);
void handle_wifi_disconnect_result(struct net_mgmt_event_callback* cb);

void wifi_mgmt_event_handler(struct net_mgmt_event_callback* cb,
			     uint32_t mgmt_event, struct net_if* netif);
void wifi_mgmt_event_init(void);

void wifi_connect(void);

#endif // _WIFI_CONNECT_H_
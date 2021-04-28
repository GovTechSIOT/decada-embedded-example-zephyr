#ifndef _DNS_LOOKUP_H_
#define _DNS_LOOKUP_H_

#include <zephyr.h>
#include <net/dns_resolve.h>

#define DNS_TIMEOUT (10 * MSEC_PER_SEC)

extern struct k_poll_signal dns_signal;
extern struct k_poll_event dns_events[];

extern struct dns_addrinfo resolved_addrinfo;

void dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo* info,
		   void* user_data);
void dns_ipv4_lookup(const char* query);

#endif // _DNS_LOOKUP_H_
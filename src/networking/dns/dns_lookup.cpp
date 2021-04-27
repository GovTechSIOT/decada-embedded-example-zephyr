#include <logging/log.h>
LOG_MODULE_REGISTER(dns_lookup, LOG_LEVEL_DBG);

#include "dns_lookup.h"

struct k_poll_signal dns_signal;
struct k_poll_event dns_events[] = {
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &dns_signal)
};

struct dns_addrinfo resolved_addrinfo;

/**
 * @brief	Callback for DNS query
 * @author	Lee Tze Han
 * @param	status		Status of query
 * @param	info		Result of query
 * @param	user_data	User data provided in dns_get_addr_info
 * @note	This function can be called multiple times with DNS_EAI_INPROGRESS for each address resolved
 */
void dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo* info, void* user_data)
{
	switch (status) {
	case DNS_EAI_CANCELED:
		LOG_INF("DNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		LOG_INF("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		LOG_INF("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		LOG_INF("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS: ;
		unsigned int dns_done;
		k_poll_signal_check(&dns_signal, &dns_done, NULL);
		if (dns_done == 0) {
			/* Callback may be called multiple times; just use the first result */
			resolved_addrinfo = *info;
			k_poll_signal_raise(&dns_signal, 0);
		}
		return;
	default:
		LOG_INF("DNS resolving error (%d)", status);
		return;
	}
}

/**
 * @brief	Wrapper for DNS lookup (IPv4)
 * @author	Lee Tze Han
 * @param	query		Address to be resolved
 */
void dns_ipv4_lookup(const char* query)
{
	int rc = dns_get_addr_info(query, DNS_QUERY_TYPE_A, NULL, dns_result_cb, (void*)query, DNS_TIMEOUT);
	if (rc < 0) {
		LOG_WRN("Failed to start DNS query: %d", rc);
		return;
	}
	else {
		LOG_INF("Resolving hostname %s...", query);
	}
}
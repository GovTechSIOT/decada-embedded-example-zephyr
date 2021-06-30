#include <logging/log.h>
LOG_MODULE_REGISTER(dns_lookup, LOG_LEVEL_DBG);

#include <net/socket.h>
#include "dns_lookup.h"

#define DNS_TIMEOUT	 (4 * MSEC_PER_SEC)
#define DNS_MAX_ATTEMPTS (3)

void dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo* info, void* user_data);

DnsLookup::DnsLookup(const std::string& domain_name) : query_(domain_name)
{
	/* Setup signal and events */
	k_poll_signal_init(&resolved_signal_);
	resolved_events_[0] = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &resolved_signal_);

	/* IPv4 query */
	dns_ipv4_lookup();
}

/**
 * @brief	Get the resolved socket infomration
 * @author	Lee Tze Han
 * @return	IPv4 socket information
 * @note 	This function will block until the query is completed
 */
struct sockaddr_in DnsLookup::get_sockaddr_in(void)
{
	k_poll(resolved_events_, 1, K_FOREVER);
	struct sockaddr_in* addr = (struct sockaddr_in*)&resolved_addrinfo_.ai_addr;
	return *addr;
}

/**
 * @brief	Get the resolved IP address string
 * @author	Lee Tze Han
 * @return	IP address string
 */
std::string DnsLookup::get_ipaddr(void)
{
	char ipaddr[INET_ADDRSTRLEN];
	struct sockaddr_in addr = get_sockaddr_in();
	inet_ntop(AF_INET, &addr.sin_addr, ipaddr, INET_ADDRSTRLEN);

	return ipaddr;
}

/**
 * @brief	Returns the pointer to signal object representing resolution state
 * @author	Lee Tze Han
 * @return	Pointer to signal object
 */
struct k_poll_signal* DnsLookup::get_signal(void)
{
	return &resolved_signal_;
}

/**
 * @brief	Set the DNS query result
 * @author	Lee Tze Han
 * @param	info	Result from DNS callback
 * @return	Pointer to signal object
 */
void DnsLookup::set_resolved(struct dns_addrinfo info)
{
	resolved_addrinfo_ = info;
}

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
	/* user_data contains the DnsLookup calling context */
	DnsLookup* dns_lookup = static_cast<DnsLookup*>(user_data);

	switch (status) {
	case DNS_EAI_CANCELED:
		LOG_INF("DNS query was canceled");
		/* Probably timed out */
		dns_lookup->dns_ipv4_lookup();
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

	case DNS_EAI_INPROGRESS:;
		unsigned int dns_done;
		k_poll_signal_check(dns_lookup->get_signal(), &dns_done, NULL);

		if (dns_done == 0) {
			/* Callback may be called multiple times; just use the first result */
			dns_lookup->set_resolved(*info);
			k_poll_signal_raise(dns_lookup->get_signal(), 0);
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
 * @note	This function will try to request for a DNS query a maximum of DNS_MAX_ATTEMPTS.
 * 		Attempts can be started from the callback if previous tries have timed out
 */
void DnsLookup::dns_ipv4_lookup(void)
{
	if (attempt_ > DNS_MAX_ATTEMPTS) {
		LOG_ERR("DNS query failed");
		return;
	}

	int rc = dns_get_addr_info(query_.c_str(), DNS_QUERY_TYPE_A, NULL, dns_result_cb, (void*)this, DNS_TIMEOUT);
	attempt_++;

	if (rc < 0) {
		LOG_WRN("Failed to start DNS query: %d", rc);

		/* Try again after 100ms */
		k_msleep(100);
		dns_ipv4_lookup();

		return;
	}
	else {
		LOG_INF("Resolving hostname %s... (Attempt %d/%d)", query_.c_str(), attempt_, DNS_MAX_ATTEMPTS);
	}
}
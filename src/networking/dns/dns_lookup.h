#ifndef _DNS_LOOKUP_H_
#define _DNS_LOOKUP_H_

#include <string>
#include <zephyr.h>
#include <net/dns_resolve.h>

class DnsLookup
{
public:
	DnsLookup(std::string domain_name);

	void dns_ipv4_lookup(void);

	struct sockaddr_in get_sockaddr_in(void);
	std::string get_ipaddr(void);
	struct k_poll_signal* get_signal(void);

	void set_resolved(struct dns_addrinfo addrinfo);

private:
	struct k_poll_signal resolved_signal_;
	struct k_poll_event resolved_events_[1];

	int attempt_ = 0;
	std::string query_;
	struct dns_addrinfo resolved_addrinfo_;
};

#endif // _DNS_LOOKUP_H_
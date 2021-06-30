/*******************************************************************************************************
 * Copyright (c) 2021 Government Technology Agency of Singapore (GovTech)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 *
 * See the License for the specific language governing permissions and limitations under the License.
 *******************************************************************************************************/
#ifndef _DNS_LOOKUP_H_
#define _DNS_LOOKUP_H_

#include <string>
#include <zephyr.h>
#include <net/dns_resolve.h>

class DnsLookup
{
public:
	explicit DnsLookup(const std::string& domain_name);

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
#include <logging/log.h>
LOG_MODULE_REGISTER(time_manager, LOG_LEVEL_DBG);

#include <net/sntp.h>
#include <time.h>
#include "networking/dns/dns_lookup.h"
#include "time_manager.h"
#include "user_config.h"

#define SNTP_TIMEOUT (10 * MSEC_PER_SEC)

/**
 * @brief	Syncs RTC to timestamp from SNTP query
 * @author	Lee Tze Han
 * @return	Success status
 */
bool TimeManager::sync_sntp_rtc(void)
{
	/* Resolve SNTP server hostname */
	DnsLookup dns_lookup(USER_CONFIG_SNTP_SERVER_ADDR);
	std::string ipaddr = dns_lookup.get_ipaddr();

	struct sntp_time sntp_time;

	int rc = sntp_simple(ipaddr.c_str(), SNTP_TIMEOUT, &sntp_time);
	if (rc < 0) {
		LOG_WRN("Failure in SNTP query: %d", rc);
		return false;
	}

	LOG_INF("SNTP timestamp: %" PRIu64, sntp_time.seconds);

	LOG_DBG("Before sync: %" PRId64, get_timestamp());

	/* Update RTC */
	update_rtc_time(sntp_time.seconds);

	LOG_DBG("After sync: %" PRId64, get_timestamp());

	return true;
}
#include <logging/log.h>
LOG_MODULE_REGISTER(communications_thread, LOG_LEVEL_DBG);

#include <net/sntp.h>
#include "networking/dns/dns_lookup.h"
#include "networking/wifi/wifi_connect.h"
#include "threads.h"
#include "user_config.h"

#define SNTP_TIMEOUT (10 * MSEC_PER_SEC)

/**
 * 	@brief	Wrapper to perform SNTP query with a specified server address
 * 	@author	Lee Tze Han
 * 	@param 	sntp_ipaddr	IP address of NTP server
 * 	@note 	The UDP port number is taken to be 123 (RFC-1305)
 */
void sntp_query(const char* sntp_ipaddr)
{
	struct sntp_time sntp_time;

	int rc = sntp_simple(sntp_ipaddr, SNTP_TIMEOUT, &sntp_time);
	if (rc < 0) {
		LOG_WRN("Failure in SNTP query: %d", rc);
		return;
	}

	LOG_INF("SNTP timestamp: %" PRIu32, (uint32_t)sntp_time.seconds);
}

/**
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void execute_communications_thread(const char* my_name, struct k_sem* my_sem,
				   struct k_sem* other_sem)
{
	const char* tname;
	uint8_t cpu;
	struct k_thread* current_thread;

	const int wait_time_us = 100000;
	const int sleep_time_ms = 500;

	k_poll_signal_init(&wifi_signal);

	/* Setup WiFi connection */
	wifi_mgmt_event_init();
	wifi_connect();

	/* Block until WiFi connection is established */
	k_poll(wifi_events, 1, K_FOREVER);

	/* Resolve SNTP server hostname */
	DnsLookup dns_lookup(USER_CONFIG_SNTP_SERVER_ADDR);
	std::string ipaddr = dns_lookup.get_ipaddr();

	LOG_DBG("Resolved address as %s", ipaddr.c_str());
	sntp_query(ipaddr.c_str());

	while (true) {
		k_sem_take(my_sem, K_FOREVER);

		/* Thread Logic */
		current_thread = k_current_get();
		tname = k_thread_name_get(current_thread);
#if CONFIG_SMP
		cpu = arch_curr_cpu()->id;
#else
		cpu = 0;
#endif
		if (tname == NULL) {
			printk("%s: Thread initialized from cpu %d on %s!\n",
			       my_name, cpu, CONFIG_BOARD);
		}
		else {
			printk("%s: Thread initialized from cpu %d on %s!\n",
			       tname, cpu, CONFIG_BOARD);
		}

		/* End this thread loop */
		k_busy_wait(wait_time_us);
		k_msleep(sleep_time_ms);
		k_sem_give(other_sem);
	}
}
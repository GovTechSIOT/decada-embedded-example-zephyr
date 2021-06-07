#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include "device_uuid/device_uuid.h"
#include "threads/threads.h"
#include "watchdog_config/watchdog_config.h"

#define PIN_THREADS (IS_ENABLED(CONFIG_SMP) && IS_ENABLED(CONFIG_SCHED_CPU_MASK) && (CONFIG_MP_NUM_CPUS > 1))
#define STACK_SIZE  4096
#define PRIORITY    7

/* Initialize global device UUID */
const std::string device_uuid = read_device_uuid();

/* Thread Configurations */
K_THREAD_STACK_DEFINE(communications_thread_stack_area, STACK_SIZE * 8);
K_THREAD_STACK_DEFINE(behavior_manager_thread_stack_area, STACK_SIZE);
static struct k_thread communications_thread_data;
static struct k_thread behavior_manager_thread_data;

void behavior_manager_thread(void* dummy1, void* dummy2, void* dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	execute_behavior_manager_thread();
}

void communications_thread(void* dummy1, void* dummy2, void* dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	execute_communications_thread();
}

void main(void)
{
	k_poll_signal_init(&watchdog_signal);

	/* Spawn communications_thread */
	k_thread_create(&communications_thread_data, communications_thread_stack_area,
			K_THREAD_STACK_SIZEOF(communications_thread_stack_area), communications_thread, NULL, NULL,
			NULL, PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&communications_thread_data, "communications_thread");
#if PIN_THREADS
	k_thread_cpu_mask_clear(&communications_thread_data);
	k_thread_cpu_mask_enable(&communications_thread_data, 0);
#endif

	/* Spawn behavior_manager_thread */
	k_thread_create(&behavior_manager_thread_data, behavior_manager_thread_stack_area,
			K_THREAD_STACK_SIZEOF(behavior_manager_thread_stack_area), behavior_manager_thread, NULL, NULL,
			NULL, PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&behavior_manager_thread_data, "behavior_manager_thread");
#if PIN_THREADS
	k_thread_cpu_mask_clear(&behavior_manager_thread_data);
	k_thread_cpu_mask_enable(&behavior_manager_thread_data, 1);
#endif

	/* Start threads */
	k_thread_start(&communications_thread_data);
	k_thread_start(&behavior_manager_thread_data);

	/* Wait for all threads to add their watchdogs before starting it */
	k_poll(watchdog_events, 1, K_FOREVER);
	watchdog_config::start_watchdog();

	k_sleep(K_FOREVER);
}

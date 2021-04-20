#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include "../threads/threads.h"

#define PIN_THREADS (IS_ENABLED(CONFIG_SMP)		  \
		     && IS_ENABLED(CONFIG_SCHED_CPU_MASK) \
		     && (CONFIG_MP_NUM_CPUS > 1))
#define STACKSIZE 1024
#define PRIORITY 7

/* Thread Configurations */
K_THREAD_STACK_DEFINE(communications_thread_stack_area, STACKSIZE);
static struct k_thread communications_thread_data;
K_THREAD_STACK_DEFINE(behavior_manager_thread_stack_area, STACKSIZE);
static struct k_thread behavior_manager_thread_data;
K_SEM_DEFINE(communications_thread_sem, 1, 1);		/* starts off "available" */
K_SEM_DEFINE(behavior_manager_thread_sem, 0, 1);	/* starts off "not available" */

void BehaviorManagerThread(void * dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
    ARG_UNUSED(dummy2);
    ARG_UNUSED(dummy3);

	ExecuteBehaviorManagerThread(__func__, &behavior_manager_thread_sem, &communications_thread_sem);
}

void CommunicationsThread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	/* invoke routine to ping-pong hello messages with threadB */
	ExecuteCommunicationsThread(__func__, &communications_thread_sem, &behavior_manager_thread_sem);
}

void main(void)
{
	/* Spawn CommunicationsThread */
	k_thread_create(&communications_thread_data, communications_thread_stack_area,
			K_THREAD_STACK_SIZEOF(communications_thread_stack_area),
			CommunicationsThread, NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&communications_thread_data, "CommunicationsThread");
#if PIN_THREADS
	k_thread_cpu_mask_clear(&communications_thread_data);
	k_thread_cpu_mask_enable(&communications_thread_data, 0);
#endif

	/* Spawn BehaviorManagerThread */
	k_thread_create(&behavior_manager_thread_data, behavior_manager_thread_stack_area,
			K_THREAD_STACK_SIZEOF(behavior_manager_thread_stack_area),
			BehaviorManagerThread, NULL, NULL, NULL,
			PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&behavior_manager_thread_data, "BehaviorManagerThread");
#if PIN_THREADS
	k_thread_cpu_mask_clear(&behavior_manager_thread_data);
	k_thread_cpu_mask_enable(&behavior_manager_thread_data, 1);
#endif

	/* Start Threads */
	k_thread_start(&communications_thread_data);
	k_thread_start(&behavior_manager_thread_data);
}

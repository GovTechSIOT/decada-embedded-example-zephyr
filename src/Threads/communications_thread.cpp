#include "threads.h"

/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void execute_communications_thread(const char *my_name, struct k_sem *my_sem, struct k_sem *other_sem)
{
	const char *tname;
	uint8_t cpu;
	struct k_thread *current_thread;

	const int wait_time_us = 100000;
	const int sleep_time_ms = 500;

	while (true) 
	{
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
		} else {
			printk("%s: Thread initialized from cpu %d on %s!\n",
				tname, cpu, CONFIG_BOARD);
		}

		/* End this thread loop */
		k_busy_wait(wait_time_us);
		k_msleep(sleep_time_ms);
		k_sem_give(other_sem);
	}
}
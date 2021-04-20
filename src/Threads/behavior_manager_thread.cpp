#include "threads.h"
#include <drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN0	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS0	DT_GPIO_FLAGS(LED0_NODE, gpios)
#define LED1	DT_GPIO_LABEL(LED1_NODE, gpios)
#define PIN1	DT_GPIO_PIN(LED1_NODE, gpios)
#define FLAGS1	DT_GPIO_FLAGS(LED1_NODE, gpios)
#define LED2	DT_GPIO_LABEL(LED2_NODE, gpios)
#define PIN2	DT_GPIO_PIN(LED2_NODE, gpios)
#define FLAGS2	DT_GPIO_FLAGS(LED2_NODE, gpios)

/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void ExecuteBehaviorManagerThread(const char *my_name, struct k_sem *my_sem, struct k_sem *other_sem)
{
	const char *tname;
	uint8_t cpu;
	struct k_thread *current_thread;
	
	const int wait_time_us = 100000;
	const int sleep_time_ms = 500;

	/* Init GPIO LEDs */
	const struct device* led0;
	const struct device* led1;
	const struct device* led2;
	int ret;
	led0 = device_get_binding(LED0);
	led1 = device_get_binding(LED1);
	led2 = device_get_binding(LED2);
	if (led0 == NULL || led1 == NULL || led2 == NULL) 
	{
		return;
	}
	bool led_is_on[] = {true, true, true};
	const struct device* led_arr[] = {led0, led1, led2};
	int pin_arr[] = {PIN0, PIN1, PIN2};
	int flags_arr[] = {FLAGS0, FLAGS1, FLAGS2};
	int current_led_id = 0;
	for (int i = 0; i < 3; i++) {
		ret = gpio_pin_configure(led_arr[i], pin_arr[i], GPIO_OUTPUT_ACTIVE | flags_arr[i]);
		if (ret < 0) {
			return;
		}
	}

	while (true) 
	{
		k_sem_take(my_sem, K_FOREVER);

		/* Thread Logic */
		gpio_pin_set(led_arr[current_led_id], pin_arr[current_led_id], (int)led_is_on[current_led_id]);
		led_is_on[current_led_id] = !led_is_on[current_led_id];
		current_led_id = (current_led_id+1) % 3;

		/* End this thread loop */
		k_busy_wait(wait_time_us);
		k_msleep(sleep_time_ms);
		k_sem_give(other_sem);
	}
}
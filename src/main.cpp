#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define SLEEP_TIME_MS   250

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

void main(void)
{
	const struct device* led0;
	const struct device* led1;
	const struct device* led2;
	
	int ret;

	led0 = device_get_binding(LED0);
	led1 = device_get_binding(LED1);
	led2 = device_get_binding(LED2);
	if (led0 == NULL || led1 == NULL || led2 == NULL) {
		return;
	}

	bool led_is_on[] = {true, true, true};
	const struct device* led_arr[] = {led0, led1, led2};
	int pin_arr[] = {PIN0, PIN1, PIN2};
	int flags_arr[] = {FLAGS0, FLAGS1, FLAGS2};
	
	for (int i = 0; i < 3; i++) {
		ret = gpio_pin_configure(led_arr[i], pin_arr[i], GPIO_OUTPUT_ACTIVE | flags_arr[i]);
		if (ret < 0) {
			return;
		}
	}

	int i = 0;
    while (true) {
		gpio_pin_set(led_arr[i], pin_arr[i], (int)led_is_on[i]);
		led_is_on[i] = !led_is_on[i];
		i = (i+1) % 3;

		k_msleep(SLEEP_TIME_MS);
        printk("Hello World\r\n");
    }
}

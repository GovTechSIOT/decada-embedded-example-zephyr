#ifndef _WATCHDOG_CONFIG_H_
#define _WATCHDOG_CONFIG_H_

#include <drivers/watchdog.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_INST(0, st_stm32_watchdog)
#endif
#ifdef WDT_NODE
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#else
#define WDT_DEV_NAME ""
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif

#ifndef WDT_MAX_WINDOW
#define WDT_MAX_WINDOW 5000U
#endif

namespace watchdog_config
{
const struct device* get_device_instance(void);
void set_watchdog_config(wdt_timeout_cfg& wdt_config);
int add_watchdog(wdt_timeout_cfg& wdt_config);
void start_watchdog(void);
} // namespace watchdog_config

#endif // _WATCHDOG_CONFIG_H_
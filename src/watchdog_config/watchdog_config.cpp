#include <logging/log.h>
LOG_MODULE_REGISTER(watchdog_config, LOG_LEVEL_DBG);

#include "watchdog_config.h"

/**
 * @brief	Get hardware device binding.
 * @author	Lau Lee Hong
 * @return      device structure
 */
const struct device* watchdog_config::get_device_instance(void)
{
	const struct device* wdt;
	wdt = device_get_binding(WDT_DEV_NAME);
	if (!wdt) {
		LOG_ERR("Cannot get WDT device");
	}

	return wdt;
}

/**
 * @brief	Configure the watchdog configuration instance to reset after WDT_MAX_WINDOW without feed.
 * @author	Lau Lee Hong
 * @param       wdt_config      Watchdog configuration instance
 */
void watchdog_config::set_watchdog_config(wdt_timeout_cfg& wdt_config)
{
	wdt_config.flags = WDT_FLAG_RESET_SOC;
	wdt_config.window.min = 0U;
	wdt_config.window.max = WDT_MAX_WINDOW_MS;
}

/**
 * @brief	Add a watchdog and raise watchdog event flag once.
 * @author	Lau Lee Hong
 * @param       wdt_config      watchdog config.
 */
int watchdog_config::add_watchdog(wdt_timeout_cfg& wdt_config)
{
	int wdt_channel_id = wdt_install_timeout(watchdog_config::get_device_instance(), &wdt_config);
	if (wdt_channel_id < 0) {
		LOG_ERR("Watchdog install error %d", wdt_channel_id);
	}
	else {
		LOG_INF("Watchdog timeout installation success - channel %d", wdt_channel_id);
	}
	return wdt_channel_id;
}

/**
 * @brief	Start global hardware watchdog.
 * @details     Any calls to add_watchdog after this function is called will have no effect.
 * @author	Lau Lee Hong
 */
void watchdog_config::start_watchdog(void)
{
	int rc = wdt_setup(watchdog_config::get_device_instance(), WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (rc < 0) {
		LOG_ERR("Watchdog setup error %d", rc);
	}
	else {
		LOG_INF("Watchdog setup success");
	}

	return;
}
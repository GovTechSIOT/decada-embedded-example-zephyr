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
#ifndef _WATCHDOG_CONFIG_H_
#define _WATCHDOG_CONFIG_H_

#include <device.h>
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

#ifndef WDT_MAX_WINDOW_MS
#define WDT_MAX_WINDOW_MS (20 * MSEC_PER_SEC)
#endif

namespace watchdog_config
{
const struct device* get_device_instance(void);
void set_watchdog_config(wdt_timeout_cfg& wdt_config);
int add_watchdog(wdt_timeout_cfg& wdt_config);
void start_watchdog(void);
} // namespace watchdog_config

#endif // _WATCHDOG_CONFIG_H_
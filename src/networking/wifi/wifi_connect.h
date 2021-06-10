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
#ifndef _WIFI_CONNECT_H_
#define _WIFI_CONNECT_H_

#include <zephyr.h>
#include <net/wifi_mgmt.h>

extern struct k_poll_signal wifi_signal;
extern struct k_poll_event wifi_events[];

void handle_wifi_connect_result(struct net_mgmt_event_callback* cb);
void handle_wifi_disconnect_result(struct net_mgmt_event_callback* cb);

void wifi_mgmt_event_handler(struct net_mgmt_event_callback* cb, uint32_t mgmt_event, struct net_if* netif);
void wifi_mgmt_event_init(void);

void wifi_connect(void);

#endif // _WIFI_CONNECT_H_
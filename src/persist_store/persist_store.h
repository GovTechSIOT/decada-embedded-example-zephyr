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
#ifndef _PERSIST_STORE_H_
#define _PERSIST_STORE_H_

#include <string>
#include <zephyr.h>

void init_persist_storage(void);

void write_sw_ver(const std::string sw_ver);
void write_client_certificate(const std::string cert);
void write_client_certificate_serial_number(const std::string cert_sn);
void write_client_private_key(const std::string private_key);

std::string read_sw_ver(void);
std::string read_client_certificate(void);
std::string read_client_certificate_serial_number(void);
std::string read_client_private_key(void);

#endif // _PERSIST_STORE_H_
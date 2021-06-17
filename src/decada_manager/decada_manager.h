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
#ifndef _DECADA_MANAGER_H_
#define _DECADA_MANAGER_H_

#include <string>
#include "crypto_engine/crypto_engine.h"
#include "networking/mqtt/mqtt_client.h"
#include "time_engine/time_engine.h"
#include "user_config.h"

class DecadaManager : public CryptoEngine, public MqttClient
{
public:
	DecadaManager(const int channel_id);

	bool connect(void);

private:
	csr_sign_resp sign_csr(std::string csr) override;

	void subscription_callback(uint8_t* data, int len) override;
	void send_service_response(std::string message_id, std::string method, std::string response);

	/* DECADA Provisioning */
	std::string get_access_token(void);
	std::string get_device_secret(void);
	std::string create_device_in_decada(const std::string name);
	std::string check_device_creation(void);

	std::string device_secret_;

	const std::string decada_ou_id_ = USER_CONFIG_DECADA_OU_ID;
	const std::string decada_product_key_ = USER_CONFIG_DECADA_PRODUCT_KEY;
	const std::string decada_access_key_ = USER_CONFIG_DECADA_ACCESS_KEY;
	const std::string decada_access_secret_ = USER_CONFIG_DECADA_ACCESS_SECRET;

	TimeEngine time_engine_;
};

#endif // _DECADA_MANAGER_H_
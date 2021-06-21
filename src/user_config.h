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
#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

// clang-format off

/**
 *      WiFi Details
 */

// Maximum of 32 characters
#define USER_CONFIG_WIFI_SSID \
        ("")

// Between 8 - 64 characters
#define USER_CONFIG_WIFI_PASS \
        ("")

/**
 *      SNTP Server address
 */

#define USER_CONFIG_SNTP_SERVER_ADDR \
        ("pool.ntp.org")

/**
 *      Device Provisioning Key Generation
 */

// Use ECC SECP256R1 as key generation method; Comment out the next line to use RSA.
#define USER_CONFIG_USE_ECC_SECP256R1

/**
 *      DECADA Credentials
 */

// Organization Unit ID for DECADA cloud
#define USER_CONFIG_DECADA_OU_ID \
        ("")

// Access key for DECADA cloud application
#define USER_CONFIG_DECADA_ACCESS_KEY \
        ("")

// Access secret for DECADA cloud application
#define USER_CONFIG_DECADA_ACCESS_SECRET \
        ("")

// Product key for DECADA cloud product
#define USER_CONFIG_DECADA_PRODUCT_KEY \
        ("")

// Product secret for DECADA cloud product
#define USER_CONFIG_DECADA_PRODUCT_SECRET \
        ("")

// clang-format on

#endif // _USER_CONFIG_H_
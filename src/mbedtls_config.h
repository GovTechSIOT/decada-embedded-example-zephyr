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
#include "user_config.h"

#if defined(USER_CONFIG_USE_ECC_SECP256R1)
#define MBEDTLS_ECP_C
#else
// Enable RSA key generation
#define MBEDTLS_RSA_C
#endif
#define MBEDTLS_GENPRIME

// Enable creating X509 CSRs
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CSR_WRITE_C
#define MBEDTLS_PEM_WRITE_C

#define MBEDTLS_DEBUG_C
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
#ifndef _CRYPTO_ENGINE_H_
#define _CRYPTO_ENGINE_H_

#include <string>
#include <drivers/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>

/* Expected response from signing CSR */
typedef struct {
	/* Indicates if struct contains valid values */
	bool valid;
	/* Certificate from CA signing CSR */
	std::string cert;
	/* Serial number of issued certificate */
	std::string cert_sn;
} csr_sign_resp;

class CryptoEngine
{
public:
	CryptoEngine(void);
	virtual ~CryptoEngine(void);

	static bool get_cert_info(char* buf, size_t size, const mbedtls_x509_crt* crt);

protected:
	mbedtls_pk_context pk_ctx_;
	std::string csr_;

private:
	bool generate_keypair(void);
	std::string generate_csr(void);
	std::string make_subject_name(void);

	virtual csr_sign_resp sign_csr(std::string csr) = 0;

	const char* mbedtls_pers_ = "gen_key";
	const std::string cert_subject_base_ = "C=SG, ST=Singapore, L=Singapore, O=DECADA, OU=DECADA CA, CN=";

	mbedtls_rsa_context rsa_keypair_;
	mbedtls_ctr_drbg_context ctrdrbg_ctx_;

	const struct device* entropy_device_;
};

#endif // _CRYPTO_ENGINE_H_
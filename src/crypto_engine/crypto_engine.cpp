#include <logging/log.h>
LOG_MODULE_REGISTER(crypto_engine, LOG_LEVEL_DBG);

#include <mbedtls/pk_internal.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_csr.h>
#include <net/tls_credentials.h>
#include "crypto_engine.h"
#include "device_uuid/device_uuid.h"
#include "persist_store/persist_store.h"
#include "time_engine/time_engine.h"
#include "tls_certs.h"
#include "user_config.h"
#include "watchdog_config/watchdog_config.h"
#if defined(USER_CONFIG_USE_ECC_SECP256R1)
#include "mbedtls/ecdsa.h"
#endif // USER_CONFIG_USE_ECC_SECP256R1

#if !defined(USER_CONFIG_USE_ECC_SECP256R1)
/* RSA key values */
#define MBEDTLS_KEY_SIZE (2048)
#define MBEDTLS_EXPONENT (65537)
#endif // USER_CONFIG_USE_ECC_SECP256R1

std::string session_client_key;

int trng_entropy_func(void* ctx, unsigned char* buf, size_t len);

CryptoEngine::CryptoEngine(const int wdt_channel_id) : wdt_channel_id_(wdt_channel_id)
{
	wdt_ = watchdog_config::get_device_instance();

	/* Initialize device object providing entropy */
	entropy_device_ = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	if (!entropy_device_) {
		LOG_ERR("Failed to obtain entropy device");
		return;
	}

#if defined(USER_CONFIG_USE_ECC_SECP256R1)
	mbedtls_ecp_keypair_init(&ecp_keypair_);
#else
	mbedtls_rsa_init(&rsa_keypair_, MBEDTLS_RSA_PKCS_V15, 0);
#endif // USER_CONFIG_USE_ECC_SECP256R1

	mbedtls_pk_init(&pk_ctx_);
	mbedtls_ctr_drbg_init(&ctrdrbg_ctx_);

	/* Seed PRNG on start of CryptoEngine lifecycle */
	int rc = mbedtls_ctr_drbg_seed(&ctrdrbg_ctx_, trng_entropy_func, (void*)entropy_device_,
				       (const unsigned char*)mbedtls_pers_, strlen(mbedtls_pers_));
	if (rc) {
		LOG_ERR("mbedtls_ctr_drbg_seed failed: -0x%04X", -rc);
		return;
	}
}

CryptoEngine::~CryptoEngine(void)
{
	entropy_device_ = NULL;

	mbedtls_pk_free(&pk_ctx_);
	mbedtls_ctr_drbg_free(&ctrdrbg_ctx_);

#if defined(USER_CONFIG_USE_ECC_SECP256R1)
	mbedtls_ecp_keypair_free(&ecp_keypair_);
#else
	mbedtls_rsa_free(&rsa_keypair_);
#endif // USER_CONFIG_USE_ECC_SECP256R1
}

/**
 *  @brief  	Return a signed client certificate
 *  @author 	Lee Tze Han
 *  @return 	Returns a csr_sign_resp struct
 */
csr_sign_resp CryptoEngine::get_client_cert(void)
{
	std::string csr = generate_csr();
	if (csr == "") {
		LOG_WRN("Failed to generate CSR");
		return { .valid = false };
	}

	wdt_feed(wdt_, wdt_channel_id_);

	csr_sign_resp resp = sign_csr(csr);
	wdt_feed(wdt_, wdt_channel_id_);

	return resp;
}

/**
 *  @brief  	Generate CSR for retrieving client certificate from DECADA.
 *  @author 	Lee Tze Han
 *  @return 	Return PEM-formatted CSR
 */
std::string CryptoEngine::generate_csr(void)
{
	mbedtls_x509write_csr mbedtls_csr_request;
	unsigned char mbedtls_csr_pem[4096];

	std::string csr_subject_name = make_subject_name();

	int rc;
	std::string csr = "invalid";
	do {
		/* Always create new keypair */
		if (!generate_keypair()) {
			LOG_WRN("Failed to generate keypair");
			return "";
		}

		/* Configure CSR */
		mbedtls_x509write_csr_init(&mbedtls_csr_request);
		mbedtls_x509write_csr_set_md_alg(&mbedtls_csr_request, MBEDTLS_MD_SHA256);
		mbedtls_x509write_csr_set_key_usage(&mbedtls_csr_request, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
		mbedtls_x509write_csr_set_key(&mbedtls_csr_request, &pk_ctx_);

		/* Check the subject name for validity */
		rc = mbedtls_x509write_csr_set_subject_name(&mbedtls_csr_request, csr_subject_name.c_str());
		if (rc) {
			LOG_WRN("mbedtls_x509write_csr_set_subject_name failed: -0x%04X", -rc);
			break;
		}

		/* Write CSR in PEM format */
		memset(mbedtls_csr_pem, 0, sizeof(mbedtls_csr_pem));
		rc = mbedtls_x509write_csr_pem(&mbedtls_csr_request, mbedtls_csr_pem, sizeof(mbedtls_csr_pem),
					       mbedtls_ctr_drbg_random, &ctrdrbg_ctx_);
		if (rc < 0) {
			LOG_WRN("mbedtls_x509write_csr_pem failed: -0x%04X", -rc);
			break;
		}

		LOG_INF("CSR Generation Successful");
		csr = (char*)mbedtls_csr_pem;
	}
	while (false);

	mbedtls_x509write_csr_free(&mbedtls_csr_request);

	return csr;
}

/**
 *  @brief  	Generate a keypair.
 *  @author 	Lee Tze Han
 *  @return 	Success status
 */
bool CryptoEngine::generate_keypair(void)
{
	int rc;
#if defined(USER_CONFIG_USE_ECC_SECP256R1)
	LOG_DBG("Generating ECC Keypair");
	pk_ctx_.pk_ctx = &ecp_keypair_;
	pk_ctx_.pk_info = &mbedtls_eckey_info;
	rc = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(pk_ctx_), mbedtls_ctr_drbg_random,
				 &ctrdrbg_ctx_);
	if (rc != 0) {
		LOG_WRN("mbedtls_ecp_gen_key failed: -0x%04X", -rc);
		return false;
	}
#else
	LOG_DBG("Generating RSA Keypair");
	rc = mbedtls_rsa_gen_key(&rsa_keypair_, mbedtls_ctr_drbg_random, &ctrdrbg_ctx_, MBEDTLS_KEY_SIZE,
				 MBEDTLS_EXPONENT);
	if (rc != 0) {
		LOG_WRN("mbedtls_rsa_gen_key failed: -0x%04X", -rc);
		return false;
	}

	/* RSA Keypair generation on embedded devices could take 10s of seconds - crucial to feed the watchdog as soon as we can */
	wdt_feed(wdt_, wdt_channel_id_);

	pk_ctx_.pk_ctx = &rsa_keypair_;
	pk_ctx_.pk_info = &mbedtls_rsa_info;
#endif // USER_CONFIG_USE_ECC_SECP256R1

	unsigned char buf[4096];
	rc = mbedtls_pk_write_key_pem(&pk_ctx_, buf, sizeof(buf));
	if (rc != 0) {
		LOG_WRN("mbedtls_pk_write_key_pem failed: -0x%04X", -rc);
		return false;
	}

	/* TODO: Requires fix for NVS */
	// write_client_private_key((char*)buf);
	session_client_key = std::string((char*)buf);

	wdt_feed(wdt_, wdt_channel_id_);

	return true;
}

/**
 *  @brief  	Create subject name for client CSR
 *  @author 	Lee Tze Han
 *  @return 	C++ string of subject name
 */
std::string CryptoEngine::make_subject_name(void)
{
	return cert_subject_base_ + device_uuid + TimeEngine().get_timestamp_ms_str();
}

/**
 *  @brief  	Entropy callback wrapper
 *  @author 	Lee Tze Han
 *  @param	ctx	TRNG device object
 *  @param	buf	Buffer to fill
 *  @param	len	Length of buffer
 *  @return 	Error code
 */
int trng_entropy_func(void* ctx, unsigned char* buf, size_t len)
{
	return entropy_get_entropy((const device*)ctx, buf, len);
}
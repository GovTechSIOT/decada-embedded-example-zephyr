#ifndef _CRYPTO_ENGINE_H_
#define _CRYPTO_ENGINE_H_

#include <string>
#include <drivers/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>

class CryptoEngine
{
public:
	CryptoEngine(void);
	~CryptoEngine(void);

	/* TODO: For testing */
	std::string get_csr(void)
	{
		return csr_;
	}

	static bool get_cert_info(char* buf, size_t size,
				  const mbedtls_x509_crt* crt);
	static std::string hash_sha256(std::string input);

protected:
	mbedtls_pk_context pk_ctx_;
	std::string csr_;

private:
	bool generate_keypair(void);
	std::string generate_csr(void);
	std::string make_subject_name(void);

	const char* mbedtls_pers_ = "gen_key";
	const std::string cert_subject_base_ =
		"C=SG, ST=Singapore, L=Singapore, O=DECADA, OU=DECADA CA, CN=";

	mbedtls_rsa_context rsa_keypair_;
	mbedtls_ctr_drbg_context ctrdrbg_ctx_;

	const struct device* entropy_device_;
};

#endif // _CRYPTO_ENGINE_H_
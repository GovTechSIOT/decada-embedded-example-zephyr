#ifndef _DECADA_MANAGER_H_
#define _DECADA_MANAGER_H_

#include <string>
#include "ArduinoJson.hpp"
#include "crypto_engine/crypto_engine.h"
#include "time_engine/time_engine.h"
#include "user_config.h"

class DecadaManager : public CryptoEngine
{
public:
	DecadaManager(void);

	bool connect(void);

private:
	csr_sign_resp sign_csr(std::string csr) override;

	/* DECADA Provisioning */
	std::string get_access_token(void);
	std::string get_device_secret(void);
	std::string create_device_in_decada(const std::string name);
	std::string check_device_creation(void);

	std::string device_secret_;

	const std::string decada_api_url_ = USER_CONFIG_DECADA_API_URL;
	const std::string decada_ou_id_ = USER_CONFIG_DECADA_OU_ID;
	const std::string decada_product_key_ = USER_CONFIG_DECADA_PRODUCT_KEY;
	const std::string decada_access_key_ = USER_CONFIG_DECADA_ACCESS_KEY;
	const std::string decada_access_secret_ = USER_CONFIG_DECADA_ACCESS_SECRET;

	TimeEngine time_engine_;
};

#endif // _DECADA_MANAGER_H_
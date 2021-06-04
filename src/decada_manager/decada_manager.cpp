#include <logging/log.h>
LOG_MODULE_REGISTER(decada_manager, LOG_LEVEL_DBG);

#include "ArduinoJson.hpp"
#include "conversions/conversions.h"
#include "decada_manager.h"
#include "device_uuid/device_uuid.h"
#include "networking/http/https_request.h"
#include "persist_store/persist_store.h"
#include "tls_certs.h"

#define CONTENT_TYPE_JSON_UTF8 ("application/json;charset=UTF-8")

DecadaManager::DecadaManager(void)
{
	if (csr_ != "") {
		csr_sign_resp sign_resp = sign_csr(csr_);
		if (sign_resp.valid) {
			/* TODO: Requires fix for NVS */
			// write_client_certificate(sign_resp.cert);
			// write_client_certificate_serial_number(sign_resp.cert_sn);
		}
		else {
			LOG_ERR("DecadaManager has no valid client certificate");
		}
	}
}

/**
 *  @brief	Setup network connection to DECADA cloud
 *  @author	Lee Tze Han
 *  @return	Success status
 */
bool DecadaManager::connect(void)
{
	/* If device is not yet created, attempt to provision with DECADA */
	device_secret_ = check_device_creation();

	/* TODO: MQTT connection */
	return true;
}

/**
 *  @brief	Sign generated CSR through REST API
 *  @author	Lee Tze Han
 *  @param	csr	Certificate Signing Request (PEM)
 *  @return	csr_sign_resp struct containing client certificate and serial number
 */
csr_sign_resp DecadaManager::sign_csr(std::string csr)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string access_token = get_access_token();
	const std::string action_query = "actionapply";
	const std::string request_url = decada_api_url_ +
					"/connect-service/v2.0/certificates?action=apply&orgId=" + decada_ou_id_ +
					"&productKey=" + decada_product_key_ + "&deviceKey=" + device_uuid;

	ArduinoJson::DynamicJsonDocument json(4096);
	json["csr"] = csr;
	json["validDay"] = 365;
	json["timestamp"] = timestamp_ms;

	std::string json_body;
	ArduinoJson::serializeJson(json, json_body);

	const std::string params = action_query + "deviceKey" + device_uuid + "orgId" + decada_ou_id_ + "productKey" +
				   decada_product_key_ + json_body;
	const std::string signature = hash_sha256(access_token + params + timestamp_ms + decada_access_secret_);

	HttpsRequest request(request_url);
	request.add_header("Content-Type", CONTENT_TYPE_JSON_UTF8);
	request.add_header("apim-accesstoken", access_token);
	request.add_header("apim-signature", signature);
	request.add_header("apim-timestamp", timestamp_ms);

	csr_sign_resp signed_cert = { .valid = false };
	if (request.send_request(HTTP_POST, json_body)) {
		std::string response = request.get_response_body();

		json.clear();
		ArduinoJson::deserializeJson(json, response);

		auto cert = json["data"]["cert"];
		auto cert_sn = json["data"]["certSN"];
		if (!cert.isNull() && !cert_sn.isNull()) {
			return { .valid = true, .cert = cert, .cert_sn = cert_sn };
		}

		LOG_WRN("Unexpected JSON shape - received: %s", response.c_str());
	}
	else {
		LOG_WRN("Failed to send CSR signing request");
	}

	return { .valid = false };
}

/**
 *  @brief	Get access token required for REST API calls
 *  @author	Lee Tze Han
 *  @return	Access token for DECADA REST API
 */
std::string DecadaManager::get_access_token(void)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string request_url = decada_api_url_ + "/apim-token-service/v2.0/token/get";

	const std::string signature = hash_sha256(decada_access_key_ + timestamp_ms + decada_access_secret_);

	ArduinoJson::DynamicJsonDocument json(512);
	json["appKey"] = decada_access_key_;
	json["encryption"] = signature;
	json["timestamp"] = timestamp_ms;

	std::string json_body;
	ArduinoJson::serializeJson(json, json_body);

	HttpsRequest request(request_url);
	request.add_header("Content-Type", CONTENT_TYPE_JSON_UTF8);

	if (request.send_request(HTTP_POST, json_body)) {
		std::string response = request.get_response_body();

		json.clear();
		ArduinoJson::deserializeJson(json, response);

		auto access_token = json["data"]["accessToken"];
		if (!access_token.isNull()) {
			return access_token;
		}

		LOG_WRN("Unexpected JSON shape - received: %s", response.c_str());

		return "";
	}
	else {
		LOG_WRN("Failed to send access token request");

		return "";
	}
}

/**
 *  @brief	Get device secret through REST API
 *  @author	Lee Tze Han
 *  @return	Device secret in DECADA
 */
std::string DecadaManager::get_device_secret(void)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string access_token = get_access_token();
	const std::string action_query = "actionget";
	const std::string request_url = decada_api_url_ +
					"/connect-service/v2.1/devices?action=get&orgId=" + decada_ou_id_ +
					"&productKey=" + decada_product_key_ + "&deviceKey=" + device_uuid;

	const std::string params =
		action_query + "deviceKey" + device_uuid + "orgId" + decada_ou_id_ + "productKey" + decada_product_key_;
	const std::string signature = hash_sha256(access_token + params + timestamp_ms + decada_access_secret_);

	HttpsRequest request(request_url);
	request.add_header("apim-accesstoken", access_token);
	request.add_header("apim-signature", signature);
	request.add_header("apim-timestamp", timestamp_ms);

	if (request.send_request(HTTP_GET)) {
		std::string response = request.get_response_body();

		ArduinoJson::DynamicJsonDocument json(1024);
		ArduinoJson::deserializeJson(json, response);

		auto device_secret = json["data"]["deviceSecret"];
		if (!device_secret.isNull()) {
			return device_secret;
		}

		LOG_WRN("Unexpected JSON shape - received: %s", response.c_str());

		return "";
	}
	else {
		LOG_WRN("Failed to send device secret request");

		return "";
	}
}

/**
 *  @brief	Create device as an entity under the product key specified
 *  @author	Lee Tze Han
 *  @param	name	User-defined device name
 *  @return	Device secret in DECADA
 */
std::string DecadaManager::create_device_in_decada(const std::string name)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string access_token = get_access_token();
	const std::string action_query = "actioncreate";
	const std::string request_url =
		decada_api_url_ + "/connect-service/v2.1/devices?action=create&orgId=" + decada_ou_id_;

	ArduinoJson::DynamicJsonDocument json(1024);
	json["productKey"] = decada_product_key_;
	json["timezone"] = "+08:00";
	json["deviceName"]["defaultValue"] = name;
	json["deviceName"]["i18nValue"] = "";
	json["deviceKey"] = device_uuid;

	std::string json_body;
	ArduinoJson::serializeJson(json, json_body);

	const std::string params = action_query + "orgId" + decada_ou_id_ + json_body;
	const std::string signature = hash_sha256(access_token + params + timestamp_ms + decada_access_secret_);

	HttpsRequest request(request_url);
	request.add_header("Content-Type", CONTENT_TYPE_JSON_UTF8);
	request.add_header("apim-accesstoken", access_token);
	request.add_header("apim-signature", signature);
	request.add_header("apim-timestamp", timestamp_ms);

	if (request.send_request(HTTP_POST)) {
		std::string response = request.get_response_body();

		json.clear();
		ArduinoJson::deserializeJson(json, response);

		auto device_secret = json["data"]["deviceSecret"];
		if (!device_secret.isNull()) {
			return device_secret;
		}

		LOG_WRN("Unexpected JSON shape - received: %s", response.c_str());

		return "";
	}
	else {
		LOG_WRN("Failed to send create device request");

		return "";
	}
}

/**
 *  @brief	Ensures device is created in DECADA
 *  @author	Lee Tze Han
 *  @return	Device secret in DECADA
 */
std::string DecadaManager::check_device_creation(void)
{
	std::string device_secret = get_device_secret();

	/* Create device in DECADA cloud as a device entity */
	while (device_secret == "") {
		device_secret = create_device_in_decada("core-" + device_uuid);
		if (device_secret != "") {
			break;
		}

		/* Try again after 500ms */
		k_msleep(500);
	}

	LOG_INF("Device created in DECADA");

	return device_secret;
}
#include <logging/log.h>
LOG_MODULE_REGISTER(decada_manager, LOG_LEVEL_DBG);

#include "ArduinoJson.hpp"
#include <algorithm>
#include "conversions/conversions.h"
#include "decada_manager.h"
#include "device_uuid/device_uuid.h"
#include "networking/http/https_request.h"
#include "persist_store/persist_store.h"
#include "tls_certs.h"
#include "user_config.h"
#include "watchdog_config/watchdog_config.h"

#define CONTENT_TYPE_JSON_UTF8 ("application/json;charset=UTF-8")

/* Root URL for DECADA API */
const std::string decada_api_url = "https://ag.decada.gov.sg";
/* MQTT Broker hostname */
const std::string decada_mqtt_hostname = "mqtt.decada.gov.sg";
/* MQTT Broker port */
#if defined(USER_CONFIG_USE_ECC_SECP256R1)
const int decada_mqtt_port = 18887;
#else
const int decada_mqtt_port = 18885;
#endif

std::string session_client_cert;

/**
 *  @brief	Add client certificate and private key to Zephyr's secure socket layer
 *  @author	Lee Tze Han
 *  @details	This function should only be called once when credentials are available
 */
void add_tls_client_creds(void)
{
	/* Set client certificate */
	int rc = tls_credential_add(CLIENT_CERTS_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE, session_client_cert.c_str(),
				    session_client_cert.size() + 1);
	if (rc < 0) {
		if (rc == -EEXIST) {
			LOG_WRN("Client certificate already exists - add_tls_client_creds should only be called once");
		}
		else {
			LOG_WRN("Failed to set client certificate: %d", rc);
		}
	}
	else {
		LOG_INF("Successfully set client certificate");
	}

	/* Set client private key */
	rc = tls_credential_add(CLIENT_CERTS_TAG, TLS_CREDENTIAL_PRIVATE_KEY, session_client_key.c_str(),
				session_client_key.size() + 1);
	if (rc < 0) {
		if (rc == -EEXIST) {
			LOG_WRN("Client key already exists - add_tls_client_creds should only be called once");
		}
		else {
			LOG_WRN("Failed to set client private key: %d", rc);
		}
	}
	else {
		LOG_INF("Successfully set client private key");
	}
}

DecadaManager::DecadaManager(const int wdt_channel_id) : CryptoEngine(wdt_channel_id)
{
	/* If device is not yet created, attempt to provision with DECADA */
	device_secret_ = check_device_creation();
}

/**
 *  @brief	Check TLS credentials
 *  @author	Lee Tze Han
 *  @return	Validity of TLS credentials
 */
bool DecadaManager::check_credentials(void)
{
	/* TODO: Requires fix for NVS */
	// std::string client_cert = read_client_certificate();
	std::string client_cert = "";

	/* Already has valid certificate */
	if (client_cert != "") {
		LOG_DBG("Using saved client certificate");
		add_tls_client_creds();

		return true;
	}

	wdt_feed(wdt_, wdt_channel_id_);

	/* Create a new client certificate (and keypair) */
	csr_sign_resp resp = get_client_cert();
	wdt_feed(wdt_, wdt_channel_id_);

	if (resp.valid) {
		/* TODO: Requires fix for NVS */
		// write_client_certificate(sign_resp.cert);
		// write_client_certificate_serial_number(sign_resp.cert_sn)
		session_client_cert = resp.cert;
		add_tls_client_creds();

		return true;
	}
	else {
		LOG_WRN("Failed to get signed client certificate");
		return false;
	}
}

/**
 *  @brief	Setup MQTT connection to DECADA.
 *  @author	Lee Tze Han
 *  @return	Success status
 */
bool DecadaManager::connect(void)
{
	/* Ensure TLS credentials are valid */
	if (!check_credentials()) {
		LOG_ERR("DecadaManager has no valid TLS credentials");
	}

	std::string timestamp_ms = time_engine_.get_timestamp_ms_str();

	std::string id = device_uuid + "|securemode=2,signmethod=sha256,timestamp=" + timestamp_ms + "|";
	std::string username = device_uuid + "&" + decada_product_key_;
	std::string password = hash_sha256("clientId" + device_uuid + "deviceKey" + device_uuid + "productKey" +
					   decada_product_key_ + "timestamp" + timestamp_ms + device_secret_);

	mqtt_client_conf conf = { .broker_hostname = decada_mqtt_hostname,
				  .broker_port = decada_mqtt_port,
				  .id = id,
				  .username = username,
				  .password = password };

	if (!MqttClient::connect(conf)) {
		LOG_ERR("Failed to establish MQTT connection");
		return false;
	}

	return true;
}

/**
 *  @brief	Callback for parsing incoming MQTT publish messages.
 *  @author	Lee Tze Han
 *  @param	data	Binary data
 *  @param	len	Length of data
 */
void DecadaManager::subscription_callback(uint8_t* data, int len)
{
	/* A valid string is expected from DECADA */
	std::string message((char*)data, len);

	ArduinoJson::DynamicJsonDocument json(1024);
	ArduinoJson::deserializeJson(json, message);

	auto id = json["id"];
	auto method = json["method"];
	auto version = json["version"];
	auto params = json["params"];

	if (id.isNull() || method.isNull() || version.isNull() || params.isNull()) {
		LOG_WRN("Unexpected JSON shape - received: %s", message.c_str());
		return;
	}

	LOG_DBG("Received message: id = %s, method = %s", id.as<const char*>(), method.as<const char*>());

	/* Parse parameters */
	ArduinoJson::JsonObject params_map = params.as<ArduinoJson::JsonObject>();
	for (auto param_kv : params_map) {
		std::string parameter = param_kv.key().c_str();
		std::string value = param_kv.value().as<std::string>();

		LOG_INF("Parameter %s = %s", parameter.c_str(), value.c_str());

		/* Perform operations with parameters here */
		/* ... */
	}

	/* 
	 * The trace_result_name is the output parameter that DECADA expects to receive.
	 * This field is defined in the model configured for the device on DECADA.
	 *
	 * In this example, the input parameter is configured as "sensor_poll_rate" with
	 * the output parameter as "poll_rate_updated".
	 */
	if (!params_map.getMember("sensor_poll_rate").isNull()) {
		send_service_response(id, method, "poll_rate_updated");
	}
}

/**
 *  @brief	Publish a response acknowledging the message from DECADA.
 *  @author	Lee Tze Han
 *  @param	message_id		Message ID to be acknowledged
 *  @param	method			Service method
 *  @param	trace_result_name	Key name for trace result status
 *  @note	This method will be called from a workqueue thread so it has to be ensured that the stack
 *  		is sufficiently large to handle the memory required for TLS. An alternative would be to
 *  		aggregate all outgoing MQTT messages and publish only from one thread.
 */
void DecadaManager::send_service_response(std::string message_id, std::string method, std::string trace_result_name)
{
	ArduinoJson::DynamicJsonDocument json(512);
	json["id"] = message_id;
	json["code"] = 200;
	json["data"][trace_result_name] = "true";

	std::string json_body;
	ArduinoJson::serializeJson(json, json_body);

	/* Response topic */
	std::string topic_suffix = method + "_reply";
	std::replace(topic_suffix.begin(), topic_suffix.end(), '.', '/');

	std::string topic = "/sys/" + decada_product_key_ + "/" + device_uuid + "/" + topic_suffix;
	if (!publish(topic, json_body)) {
		LOG_WRN("Failed to send response for %s", method.c_str());
	}
}

/**
 *  @brief	Sign generated CSR through REST API.
 *  @author	Lee Tze Han
 *  @param	csr	Certificate Signing Request (PEM)
 *  @return	csr_sign_resp struct containing client certificate and serial number
 *  @note	DECADA requires that the device has already been created on the cloud before
 *  		a request to sign the device's CSR can be made. This is guaranteed as long
 *  		as this method is called after check_device_creation().
 */
csr_sign_resp DecadaManager::sign_csr(std::string csr)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string access_token = get_access_token();
	const std::string action_query = "actionapply";
	const std::string request_url = decada_api_url +
					"/connect-service/v2.0/certificates?action=apply&orgId=" + decada_ou_id_ +
					"&productKey=" + decada_product_key_ + "&deviceKey=" + device_uuid;

	ArduinoJson::DynamicJsonDocument json(4096);
	json["csr"] = csr;
	json["validDay"] = 365;
	json["timestamp"] = timestamp_ms;
#if defined(USER_CONFIG_USE_ECC_SECP256R1)
	json["issueAuthority"] = "ECC";
#endif // USER_CONFIG_USE_ECC_SECP256R1

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
 *  @brief	Get access token required for REST API calls.
 *  @author	Lee Tze Han
 *  @return	Access token for DECADA REST API
 */
std::string DecadaManager::get_access_token(void)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string request_url = decada_api_url + "/apim-token-service/v2.0/token/get";

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
 *  @brief	Get device secret through REST API.
 *  @author	Lee Tze Han
 *  @return	Device secret in DECADA
 */
std::string DecadaManager::get_device_secret(void)
{
	const std::string timestamp_ms = time_engine_.get_timestamp_ms_str();
	const std::string access_token = get_access_token();
	const std::string action_query = "actionget";
	const std::string request_url = decada_api_url +
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
 *  @brief	Create device as an entity under the product key specified.
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
		decada_api_url + "/connect-service/v2.1/devices?action=create&orgId=" + decada_ou_id_;

	ArduinoJson::DynamicJsonDocument json(1024);
	json["productKey"] = decada_product_key_;
	json["timezone"] = "+08:00";
	json["deviceName"]["defaultValue"] = name;
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

	if (request.send_request(HTTP_POST, json_body)) {
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
 *  @brief	Ensures device is created in DECADA.
 *  @author	Lee Tze Han
 *  @return	Device secret in DECADA
 */
std::string DecadaManager::check_device_creation(void)
{
	std::string device_secret = get_device_secret();

	/* Create device in DECADA cloud as a device entity */
	while (device_secret == "") {
		LOG_INF("Trying to create device...");
		device_secret = create_device_in_decada("zephyr-" + device_uuid);
		if (device_secret != "") {
			break;
		}

		/* Try again after 500ms */
		wdt_feed(wdt_, wdt_channel_id_);
		k_msleep(500);
	}

	LOG_INF("Device created in DECADA");

	return device_secret;
}
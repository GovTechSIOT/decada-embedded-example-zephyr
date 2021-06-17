#include <logging/log.h>
LOG_MODULE_REGISTER(mqtt_client, LOG_LEVEL_DBG);

#include <vector>
#include "device_uuid/device_uuid.h"
#include "mqtt_client.h"
#include "networking/dns/dns_lookup.h"
#include "tls_certs.h"
#include "user_config.h"

#define MQTT_CONN_RETRIES (3)
#define MQTT_TIMEOUT	  (5 * MSEC_PER_SEC)
#define MQTT_LOOP_PERIOD  K_MSEC(1 * MSEC_PER_SEC)

/* Credentials should already be added */
sec_tag_t ca_tag_list[] = { CA_CERTS_TAG, CLIENT_CERTS_TAG };

/*
 * Unfortunately the underlying MQTT library does not provide a way to pass the calling
 * MqttClient context in an mqtt_evt_cb_t (cf. dns_resolve_cb_t with a void* user_data).
 * Until such an option is available, the number of MqttClient instances is effectively
 * limited to one. This is sufficient for most purposes.
 */
static MqttClient* client_ptr;

/* Forward callbacks to instance */
void mqtt_event_handler(struct mqtt_client* client_ctx, const struct mqtt_evt* event)
{
	client_ptr->handle_event(client_ctx, event);
}

MqttClient::MqttClient(void)
{
	if (client_ptr) {
		LOG_ERR("An instance of MqttClient already exists");
	}

	k_mutex_init(&tx_mutex_);

	client_ptr = this;
}

/**
 * @brief	Establish connection to MQTT broker
 * @author	Lee Tze Han
 * @param	config	Client configuration
 * @return	Success status
 */
bool MqttClient::connect(mqtt_client_conf config)
{
	client_conf_ = config;
	username_ = { .utf8 = (uint8_t*)client_conf_.username.c_str(), .size = client_conf_.username.size() };
	password_ = { .utf8 = (uint8_t*)client_conf_.password.c_str(), .size = client_conf_.password.size() };

	/* Initialize and try to connect */
	for (int i = 0; i < MQTT_CONN_RETRIES; i++) {
		resolve_broker();
		client_setup();

		int rc = mqtt_connect(&client_ctx_);
		if (rc == 0) {
			/* Configure pollfd */
			struct pollfd fds[1];
			fds[0].fd = client_ctx_.transport.tls.sock;
			fds[0].events = POLLIN;

			/* Wait to receive from broker */
			poll(fds, 1, MQTT_TIMEOUT);
			mqtt_input(&client_ctx_);

			if (connected_) {
				start_loop();
				return true;
			}
			else {
				LOG_WRN("MQTT broker connection timed out");
			}
		}
		else {
			LOG_WRN("Failed to connect to MQTT broker: %d", rc);
		}

		/* Stop current connection */
		mqtt_abort(&client_ctx_);
	}

	return false;
}

/**
 * @brief	Disconnect from MQTT broker
 * @author	Lee Tze Han
 */
bool MqttClient::disconnect(void)
{
	int rc = mqtt_disconnect(&client_ctx_);
	if (rc < 0) {
		LOG_WRN("Failed to disconnect properly: %d", rc);
		return false;
	}
	else {
		LOG_INF("Disconnecting from broker...");
	}

	return true;
}

/**
 * @brief	Configure address for MQTT broker
 * @author	Lee Tze Han
 * @param	topic	Topic to publish to
 * @param	payload	Data to publish
 * @return	Success status
 * @details	Current implementation only supports QoS 0
 */
bool MqttClient::publish(std::string topic, std::string payload)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	param.message.topic.topic.utf8 = (uint8_t*)topic.c_str();
	param.message.topic.topic.size = topic.size();
	param.message.payload.data = (uint8_t*)payload.c_str();
	param.message.payload.len = payload.size();
	param.dup_flag = 0;
	param.retain_flag = 0;

	/*
	 * As the workqueue thread may also call this method from processing mqtt_input,
	 * the MQTT client context has to be properly guarded.
	 */
	k_mutex_lock(&tx_mutex_, K_FOREVER);

	int rc = mqtt_publish(&client_ctx_, &param);

	k_mutex_unlock(&tx_mutex_);

	if (rc < 0) {
		LOG_WRN("MQTT Publish failed: %d", rc);
		return false;
	}

	/* Not expecting PUBACK for QoS 0 */
	LOG_DBG("Successfully published to %s", topic.c_str());

	return true;
}

/**
 *  @brief	Publishes payload to specified topic
 *  @author	Lee Tze Han
 *  @param	topics	Topics to subscribe to
 *  @param	qos	MQTT QoS (QoS 0 by default)
 *  @return	Success status
 */
bool MqttClient::subscribe(const std::vector<std::string>& topics, enum mqtt_qos qos)
{
	/* Not particularly realistic, but we have to narrow later so just to be extra safe */
	if (topics.size() > UINT16_MAX) {
		LOG_ERR("Too many topics");
		return false;
	}

	std::vector<struct mqtt_topic> topic_list;
	for (size_t i = 0; i < topics.size(); i++) {
		topic_list.push_back(
			{ .topic = { .utf8 = (uint8_t*)topics[i].c_str(), .size = (uint16_t)topics[i].size() },
			  .qos = qos });
	}

	const struct mqtt_subscription_list sub_list = { .list = topic_list.data(),
							 .list_count = (uint16_t)topic_list.size(),
							 .message_id = 1 };

	int rc = mqtt_subscribe(&client_ctx_, &sub_list);
	if (rc < 0) {
		LOG_WRN("MQTT Subscribe failed: %d", rc);
		return false;
	}

	for (std::string topic : topics) {
		LOG_DBG("Subscribing to %s...", topic.c_str());
	}

	return true;
}

/**
 * @brief	Configure address for MQTT broker
 * @author	Lee Tze Han
 */
void MqttClient::resolve_broker(void)
{
	DnsLookup dns_lookup(client_conf_.broker_hostname);
	std::string ipaddr = dns_lookup.get_ipaddr();

	broker_addr_.sin_family = AF_INET;
	broker_addr_.sin_port = htons(client_conf_.broker_port);
	inet_pton(AF_INET, ipaddr.c_str(), &broker_addr_.sin_addr);
}

/**
 * @brief	Configures the MQTT client
 * @author	Lee Tze Han
 */
void MqttClient::client_setup(void)
{
	mqtt_client_init(&client_ctx_);

	/* Client configuration */
	client_ctx_.broker = &broker_addr_;
	client_ctx_.evt_cb = mqtt_event_handler;
	client_ctx_.protocol_version = MQTT_VERSION_3_1_1;
	client_ctx_.transport.type = MQTT_TRANSPORT_SECURE;

	/* TLS Configuration */
	struct mqtt_sec_config* tls_config = &client_ctx_.transport.tls.config;

	tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = ca_tag_list;
	tls_config->sec_tag_count = ARRAY_SIZE(ca_tag_list);
	tls_config->hostname = client_conf_.broker_hostname.c_str();

	client_ctx_.rx_buf = rx_buffer_;
	client_ctx_.rx_buf_size = sizeof(rx_buffer_);
	client_ctx_.tx_buf = tx_buffer_;
	client_ctx_.tx_buf_size = sizeof(tx_buffer_);

	client_ctx_.client_id.utf8 = (uint8_t*)client_conf_.id.c_str();
	client_ctx_.client_id.size = client_conf_.id.size();
	client_ctx_.user_name = &username_;
	client_ctx_.password = &password_;
}

/**
 * @brief	Receive MQTT packet
 * @author	Lee Tze Han
 * @details	This function is scheduled to run again after MQTT_LOOP_PERIOD
 */
void loop_mqtt_input(struct k_work* work_item)
{
	struct mqtt_work* mqtt_work = CONTAINER_OF(work_item, struct mqtt_work, work);

	mqtt_input(mqtt_work->client_ctx);

	k_delayed_work_submit(&mqtt_work->work, MQTT_LOOP_PERIOD);
}

/**
 * @brief	Send MQTT keep alive packet if required
 * @author	Lee Tze Han
 * @details	This function is scheduled to run again after MQTT_LOOP_PERIOD
 */
void loop_mqtt_live(struct k_work* work_item)
{
	struct mqtt_work* mqtt_work = CONTAINER_OF(work_item, struct mqtt_work, work);

	mqtt_live(mqtt_work->client_ctx);

	k_delayed_work_submit(&mqtt_work->work, MQTT_LOOP_PERIOD);
}

/**
 * @brief	Starts loop for required MQTT functions
 * @author	Lee Tze Han
 */
void MqttClient::start_loop(void)
{
	mqtt_input_work_.client_ctx = &client_ctx_;
	mqtt_live_work_.client_ctx = &client_ctx_;

	k_delayed_work_init(&mqtt_input_work_.work, loop_mqtt_input);
	k_delayed_work_init(&mqtt_live_work_.work, loop_mqtt_live);

	k_delayed_work_submit(&mqtt_input_work_.work, MQTT_LOOP_PERIOD);
	k_delayed_work_submit(&mqtt_live_work_.work, MQTT_LOOP_PERIOD);

	LOG_DBG("MQTT loop started");
}

/**
 * @brief	Stops loop of required MQTT functions
 * @author	Lee Tze Han
 */
void MqttClient::stop_loop(void)
{
	int rc;

	/* 
	 * OK if cancelling returns -EINVAL since handler does not schedule another work item;
	 * if -EALREADY is returned, then try to cancel the new task scheduled by the handler
	 */
	rc = k_delayed_work_cancel(&mqtt_input_work_.work);
	while (rc == -EALREADY) {
		rc = k_delayed_work_cancel(&mqtt_input_work_.work);
	}

	rc = k_delayed_work_cancel(&mqtt_live_work_.work);
	while (rc == -EALREADY) {
		rc = k_delayed_work_cancel(&mqtt_live_work_.work);
	}

	LOG_DBG("Terminated loop");
}

/**
 * @brief	Handles MQTT events and/or calls the appropriate handler
 * @author	Lee Tze Han
 * @param	client_ctx	mqtt_client context (same as MqttClient::client_ctx_)
 * @param	event		MQTT event details
 */
void MqttClient::handle_event(struct mqtt_client* client_ctx, const struct mqtt_evt* event)
{
	switch (event->type) {
	case MQTT_EVT_CONNACK:
		if (event->result != 0) {
			LOG_WRN("MQTT connection failed: %d", event->result);
			break;
		}

		connected_ = true;
		LOG_INF("MQTT client connected");

		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected: %d", event->result);

		connected_ = false;
		stop_loop();

		break;

	case MQTT_EVT_PUBACK:
		/* No PUBACK expected for QoS 0 */
		break;

	case MQTT_EVT_SUBACK:

		LOG_DBG("Successfully subscribed");
		break;

	case MQTT_EVT_PUBLISH:

		handle_incoming_publish(client_ctx, event);
		break;

	default:
		break;
	}
}

/**
 * @brief	Handles incoming MQTT publish message
 * @author	Lee Tze Han
 * @param	client_ctx	mqtt_client context (same as MqttClient::client_ctx_)
 * @param	event		MQTT event details
 * @details	Custom functionality can be introduced by overriding the subscription_callback function
 */
void MqttClient::handle_incoming_publish(struct mqtt_client* client_ctx, const struct mqtt_evt* event)
{
	int len = event->param.publish.message.payload.len;
	uint8_t* data = new uint8_t[len];

	int rc = mqtt_read_publish_payload(client_ctx, data, len);
	if (rc < 0) {
		LOG_WRN("Failed to read payload");
		return;
	}

	subscription_callback(data, len);
	delete[] data;

	/* QoS 0 requires no PUBACK here */
}
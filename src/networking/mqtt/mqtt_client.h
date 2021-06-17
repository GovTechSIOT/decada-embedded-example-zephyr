#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include <vector>
#include <zephyr.h>
#include <net/mqtt.h>
#include <net/socket.h>

struct mqtt_client_conf {
	/* Broker host */
	std::string broker_hostname;
	int broker_port;
	/* Client credentials */
	std::string id;
	std::string username;
	std::string password;
};

struct mqtt_work {
	struct k_delayed_work work;
	struct mqtt_client* client_ctx;
};

class MqttClient
{
public:
	MqttClient(void);
	virtual ~MqttClient(void) {}

	bool connect(mqtt_client_conf config);
	bool disconnect(void);
	bool publish(std::string topic, std::string payload);
	bool subscribe(const std::vector<std::string>& topics, enum mqtt_qos qos = MQTT_QOS_0_AT_MOST_ONCE);

	void handle_event(struct mqtt_client* client_ctx, const struct mqtt_evt* event);

private:
	void resolve_broker(void);
	void client_setup(void);

	bool connected_ = false;

	/* Periodically run MQTT functions */
	void start_loop(void);
	void stop_loop(void);

	struct mqtt_work mqtt_input_work_;
	struct mqtt_work mqtt_live_work_;

	void handle_incoming_publish(struct mqtt_client* client_ctx, const struct mqtt_evt* event);

	/* Optional callback to handle incoming MQTT publish message */
	virtual void subscription_callback(uint8_t* data, int len) {}

	struct sockaddr_in broker_addr_;
	struct mqtt_client client_ctx_;

	mqtt_client_conf client_conf_;
	struct mqtt_utf8 username_;
	struct mqtt_utf8 password_;

	/* Buffers for MQTT client */
	uint8_t rx_buffer_[256];
	uint8_t tx_buffer_[256];
	struct k_mutex tx_mutex_;
};

#endif // _MQTT_CLIENT_H_
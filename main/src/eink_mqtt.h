#pragma once

#include <functional>
#include <vector>
#include <atomic>

#include <esp_mqtt.hpp>
#include <esp_mqtt_client_config.hpp>
#include <mqtt_client.h>

struct MQTTTopicHandler {
    idf::mqtt::Filter filter;
    idf::mqtt::QoS qos;
    std::function<void(const esp_mqtt_event_handle_t)> callback;
};

class MQTTClient final : public idf::mqtt::Client {
public:
        enum ConnectionStatus {
            FAILED,
            CONNECTING,
            CONNECTED
        };

        MQTTClient(const idf::mqtt::BrokerConfiguration& broker, const idf::mqtt::ClientCredentials& credentials, const idf::mqtt::Configuration& config);

        esp_err_t register_handler(const MQTTTopicHandler& handler);
        esp_err_t set_uri(const std::string& uri);
        esp_err_t reconnect();
        esp_err_t wait_for_connection(int retries = 10);
private:
        std::vector<MQTTTopicHandler> handlers;
        std::atomic<ConnectionStatus> connection_status;

        void on_subscribed(const esp_mqtt_event_handle_t event) override;
        void on_connected(const esp_mqtt_event_handle_t event) override;
        void on_data(const esp_mqtt_event_handle_t event) override;
        void on_error(const esp_mqtt_event_handle_t event);
};
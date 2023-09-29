#pragma once

#include <functional>
#include <vector>

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
        MQTTClient(const idf::mqtt::BrokerConfiguration& broker, const idf::mqtt::ClientCredentials& credentials, const idf::mqtt::Configuration& config);
        void register_handler(const MQTTTopicHandler& handler);
private:
        std::vector<MQTTTopicHandler> handlers;

        void on_subscribed(const esp_mqtt_event_handle_t event) override;
        void on_connected(const esp_mqtt_event_handle_t event) override;
        void on_data(const esp_mqtt_event_handle_t event) override;
};
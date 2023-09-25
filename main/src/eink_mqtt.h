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
        MQTTClient(const idf::mqtt::BrokerConfiguration& broker, const idf::mqtt::ClientCredentials& credentials, const idf::mqtt::Configuration& config) = delete;
        MQTTClient(const idf::mqtt::BrokerConfiguration& broker, const idf::mqtt::ClientCredentials& credentials, const idf::mqtt::Configuration& config, const std::vector<MQTTTopicHandler>& handlers)
        : idf::mqtt::Client{broker, credentials, config}, handlers{handlers} {}

        void register_handler(const MQTTTopicHandler& handler) {
            handlers.push_back(handler);
            subscribe(const_cast<MQTTTopicHandler&>(handler).filter.get(), handler.qos);
            ESP_LOGI("MQTTClient", "Registered handler for topic %s", const_cast<MQTTTopicHandler&>(handler).filter.get().c_str());
        }

private:
        std::vector<MQTTTopicHandler> handlers;

        void on_subscribed(const esp_mqtt_event_handle_t event) override {}

        void on_connected(const esp_mqtt_event_handle_t event) override {
            for (const auto& handler : handlers) {
                subscribe(const_cast<MQTTTopicHandler&>(handler).filter.get(), handler.qos);
            }
        }
        void on_data(const esp_mqtt_event_handle_t event) override {
            for (const auto& handler : handlers) {
                if (handler.filter.match(event->topic, event->topic_len)) {
                    handler.callback(event);
                }
            }
        }
};
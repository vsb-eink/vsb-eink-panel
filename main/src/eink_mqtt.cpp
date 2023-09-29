#include "eink_mqtt.h"

#include <esp_log.h>

MQTTClient::MQTTClient(
        const idf::mqtt::BrokerConfiguration& broker,
        const idf::mqtt::ClientCredentials& credentials,
        const idf::mqtt::Configuration& config
        ) : idf::mqtt::Client{broker, credentials, config}, handlers{} {}

void MQTTClient::register_handler(const MQTTTopicHandler& handler) {
    handlers.push_back(handler);
    subscribe(const_cast<MQTTTopicHandler&>(handler).filter.get(), handler.qos);
    ESP_LOGI("MQTTClient", "Registered handler for topic %s", const_cast<MQTTTopicHandler&>(handler).filter.get().c_str());
}

void MQTTClient::on_subscribed(const esp_mqtt_event_handle_t event) {};

void MQTTClient::on_connected(const esp_mqtt_event_handle_t event) {
    for (const auto& handler : handlers) {
        subscribe(const_cast<MQTTTopicHandler&>(handler).filter.get(), handler.qos);
    }
}

void MQTTClient::on_data(const esp_mqtt_event_handle_t event) {
    for (const auto& handler : handlers) {
        if (handler.filter.match(event->topic, event->topic_len)) {
            handler.callback(event);
        }
    }
}
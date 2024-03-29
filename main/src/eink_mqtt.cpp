#include "eink_mqtt.h"

#include <esp_log.h>

MQTTClient::MQTTClient(
        const idf::mqtt::BrokerConfiguration& broker,
        const idf::mqtt::ClientCredentials& credentials,
        const idf::mqtt::Configuration& config
        ) : idf::mqtt::Client{broker, credentials, config}, handlers{}, connection_status{ConnectionStatus::CONNECTING} {}

esp_err_t MQTTClient::register_handler(const MQTTTopicHandler& handler) {
    auto message_id = subscribe(const_cast<MQTTTopicHandler&>(handler).filter.get(), handler.qos);

    if (!message_id.has_value()) {
        ESP_LOGE("MQTTClient", "Failed to register handler for topic %s", const_cast<MQTTTopicHandler&>(handler).filter.get().c_str());
        return ESP_FAIL;
    }

    handlers.push_back(handler);
    ESP_LOGI("MQTTClient", "Registered handler for topic %s", const_cast<MQTTTopicHandler&>(handler).filter.get().c_str());
    return ESP_OK;
}

esp_err_t MQTTClient::set_uri(const std::string& uri) {
    return esp_mqtt_client_set_uri(handler.get(), uri.c_str());
}

esp_err_t MQTTClient::reconnect() {
    return esp_mqtt_client_reconnect(handler.get());
}

esp_err_t MQTTClient::wait_for_connection(const int retries) {
    if (retries == 0) {
        return ESP_FAIL;
    }

    ESP_LOGI("MQTTClient", "Waiting for connection to MQTT broker");
    this->connection_status.wait(ConnectionStatus::CONNECTING);

    ESP_LOGI("MQTTClient", "Connection status: %d", this->connection_status.load());
    switch (this->connection_status.load()) {
        case ConnectionStatus::CONNECTED:
            return ESP_OK;
        case ConnectionStatus::FAILED:
            return ESP_FAIL;
        default:
            return wait_for_connection(retries - 1);
    }
}

void MQTTClient::on_subscribed(const esp_mqtt_event_handle_t event) {};

void MQTTClient::on_error(const esp_mqtt_event_handle_t event) {
    // let the base class handle the error first
    idf::mqtt::Client::on_error(event);

    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        this->connection_status = ConnectionStatus::FAILED;
        this->connection_status.notify_all();
    }
};

void MQTTClient::on_connected(const esp_mqtt_event_handle_t event) {
    this->connection_status = ConnectionStatus::CONNECTED;
    this->connection_status.notify_all();

    for (const auto& handler : handlers) {
        subscribe(const_cast<MQTTTopicHandler&>(handler).filter.get(), handler.qos);
    }
}

void MQTTClient::on_data(const esp_mqtt_event_handle_t event) {
    static auto current_topic = std::make_pair(-1, std::string{});
    if (current_topic.first != event->event_id || event->topic_len > 0) {
        current_topic = std::make_pair(event->event_id, std::string(event->topic, event->topic_len));
    }

    for (const auto& handler : handlers) {
        if (handler.filter.match(current_topic.second.begin(), current_topic.second.end())) {
            handler.callback(event);
        }
    }
}
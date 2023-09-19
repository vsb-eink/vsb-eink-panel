#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <memory>
#include <esp_mqtt.hpp>
#include <esp_mqtt_client_config.hpp>
#include <list>
#include <functional>
#include <cJSON.h>

#include "services/inkplate_static.h"

namespace mqtt = idf::mqtt;

static const char *TAG = "Main";

struct MQTTMessageHandler {
    mqtt::Filter filter;
    std::function<void(const esp_mqtt_event_handle_t)> callback;
    mqtt::QoS qos;
};

class MQTTClient final : public mqtt::Client {
public:
    using mqtt::Client::Client;

    MQTTClient(const mqtt::BrokerConfiguration& broker, const mqtt::ClientCredentials& credentials, const mqtt::Configuration& config) = delete;
    MQTTClient(const mqtt::BrokerConfiguration& broker, const mqtt::ClientCredentials& credentials, const mqtt::Configuration& config, const std::list<MQTTMessageHandler>& handlers)
            : mqtt::Client{broker, credentials, config}, handlers{handlers} {}

private:
    std::list<MQTTMessageHandler> handlers;

    void on_connected(const esp_mqtt_event_handle_t event) override{
        for (auto& handler : handlers) {
            subscribe(handler.filter.get(), handler.qos);
        }
    }
    void on_data(const esp_mqtt_event_handle_t event) override {
        for (auto& handler : handlers) {
            if (handler.filter.match(event->topic, event->topic_len)) {
                handler.callback(event);
            }
        }
    }
};

[[noreturn]]
void mainTask(void *param) {
    ESP_LOGI(TAG, "Main task has started");
    auto& inkplate = InkplateStatic::getInstance();
    inkplate.begin();

    inkplate.clearDisplay();
    inkplate.display();

    inkplate.joinAP("TurrisLukasu", "pekamalu");

    mqtt::BrokerConfiguration broker{
            .address = {mqtt::URI{std::string{"mqtt://192.168.1.164:1883"}}},
            .security =  mqtt::Insecure{}
    };
    mqtt::ClientCredentials credentials{};
    mqtt::Configuration config{};

    MQTTClient client{
            broker,
            credentials,
            config,
            {
                        {
                            mqtt::Filter{std::string{"inkplate/+/display"}},
                            [](const esp_mqtt_event_handle_t event) {
                                ESP_LOGI(TAG, "Received message on topic %.*s", event->topic_len, event->topic);
                                cJSON* message = cJSON_Parse(event->data);
                                if (message == nullptr) {
                                    ESP_LOGE(TAG, "Failed to parse message");
                                    cJSON_Delete(message);
                                    return;
                                }
                                cJSON* message_text = cJSON_GetObjectItem(message, "text");
                                if (message_text == nullptr) {
                                    ESP_LOGE(TAG, "Failed to parse message text");
                                    cJSON_Delete(message);
                                    return;
                                }
                                ESP_LOGI(TAG, "Message text: %s", message_text->valuestring);
                                ESP_LOGI(TAG, "[APP] Free memory: %lu bytes", esp_get_free_heap_size());
                                cJSON_Delete(message);
                            },
                            mqtt::QoS::AtLeastOnce
                        }
            }
    };

    while (true) {
        constexpr TickType_t xDelay = 500 / portTICK_PERIOD_MS;
        vTaskDelay(xDelay);
    }
}


#define STACK_SIZE 10000

extern "C" void app_main() {
    TaskHandle_t xHandle = nullptr;

    xTaskCreate(mainTask, "mainTask", STACK_SIZE, (void *) 1, tskIDLE_PRIORITY, &xHandle);
    configASSERT(xHandle);
}

#include <esp_crt_bundle.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <memory>
#include <esp_mqtt.hpp>
#include <esp_mqtt_client_config.hpp>

#include "services/inkplate_static.h"
#include "mqtt/topics/draw_bitmap_1bit/handler.hpp"

namespace mqtt = idf::mqtt;

static const char *TAG = "Main";

class MyClient final : public mqtt::Client {
public:
    using mqtt::Client::Client;

    MyClient(const std::string& base_topic, const mqtt::BrokerConfiguration& broker, const mqtt::ClientCredentials& credentials, const mqtt::Configuration& config)
            : mqtt::Client{broker, credentials, config}, messages(base_topic + "messages/received"), sent_load(base_topic + "load/+/sent"){}

private:
    void on_connected(const esp_mqtt_event_handle_t event) override
    {
        using mqtt::QoS;
        subscribe(messages.get());
        subscribe(sent_load.get(), QoS::AtMostOnce);
    }
    void on_data(const esp_mqtt_event_handle_t event) override
    {
        printf("Subscribed to %.*s\r\n", event->topic_len, event->topic);
        if (messages.match(event->topic, event->topic_len)) {
            ESP_LOGI(TAG, "Received in the %.*s topic", event->topic_len, event->topic);
        } else if (sent_load.match(event->topic, event->topic_len)) {
            ESP_LOGI(TAG, "Received in the sent_load topic");
        }
    }
    mqtt::Filter messages{""};
    mqtt::Filter sent_load{""};
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

    MyClient client{"$SYS/broker/", broker, credentials, config};

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

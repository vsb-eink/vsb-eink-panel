#include <thread>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_mqtt_client_config.hpp>

#include "config.h"
#include "eink_mqtt.h"
#include "drivers/inkplate_static.h"
#include "tasks/panel/panel_task.h"
#include "tasks/system/system_task.h"

namespace mqtt = idf::mqtt;

static const char *TAG = "Main";

extern "C" [[noreturn]] void app_main() {
    ESP_LOGI(TAG, "Main task has started");
    auto& inkplate = InkplateStatic::getInstance();
    inkplate.begin();

    Config config{};

    inkplate.clearDisplay();
    inkplate.display();

    inkplate.joinAP("TurrisLukasu", "pekamalu");

    mqtt::BrokerConfiguration mqtt_broker{
            .address = {mqtt::URI{std::string{"mqtt://192.168.1.164:1883"}}},
            .security =  mqtt::Insecure{}
    };
    mqtt::ClientCredentials mqtt_client_credentials{};
    mqtt::Configuration mqtt_client_config{};

    MQTTClient mqtt_client{
            mqtt_broker,
            mqtt_client_credentials,
            mqtt_client_config,
            {}
    };

    TaskContext ctx{
            .inkplate = inkplate,
            .config = config,
            .mqtt = mqtt_client
    };
    std::thread panel_task_thread(panel_task, std::ref(ctx));
    std::thread system_task_thread(system_task, std::ref(ctx));

    for (;;) {
        constexpr TickType_t xDelay = 500 / portTICK_PERIOD_MS;
        vTaskDelay(xDelay);
    }
}

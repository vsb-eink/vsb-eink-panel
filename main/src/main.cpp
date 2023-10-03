#include <thread>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_mqtt_client_config.hpp>

#include "config.h"
#include "eink_mqtt.h"
#include "drivers/inkplate_waveform.h"
#include "tasks/panel/panel_task.h"
#include "tasks/system/system_task.h"

namespace mqtt = idf::mqtt;

static const char *TAG = "Main";

extern "C" [[noreturn]] void app_main() {
    ESP_LOGI(TAG, "Main task has started");
    static Inkplate inkplate(DisplayMode::INKPLATE_3BIT);
    inkplate.begin();
    inkplate.initNVS();
    ESP_LOGI(TAG, "Inkplate initialized");

    ESP_LOGI(TAG, "Loading config from NVS");
    static Config config{};
    ESP_ERROR_CHECK(config.load_from_nvs());
    ESP_ERROR_CHECK(config.commit());
    ESP_LOGI(TAG, "Config loaded from NVS");

    ESP_LOGI(TAG, "Configuring display waveform");
    if (config.panel.waveform != 0) {
        ESP_LOGI(TAG, "Setting waveform to %d", config.panel.waveform);
        inkplate.changeWaveform(INKPLATE_WAVEFORMS[config.panel.waveform - 1]);
    }

    ESP_LOGI(TAG, "Clearing display");
    inkplate.clearDisplay();
    inkplate.display();

    ESP_LOGI(TAG, "Trying to connect to %s", config.wifi.ssid.c_str());
    auto wifi_connected = inkplate.joinAP(config.wifi.ssid.c_str(), config.wifi.password.c_str());
    if (!wifi_connected) {
        ESP_LOGE(TAG, "Failed to connect to %s", config.wifi.ssid.c_str());
        ESP_LOGW(TAG, "Using fallback network config");
        inkplate.forceDisconnect();
        wifi_connected = inkplate.joinAP(config.wifi_fallback.ssid.c_str(), config.wifi_fallback.password.c_str());

        if (!wifi_connected) {
            ESP_LOGE(TAG, "Failed to connect to fallback network");
            ESP_LOGW(TAG, "Rebooting...");
            esp_restart();
        } else {
            ESP_LOGI(TAG, "Connected to fallback SSID %s", config.wifi_fallback.ssid.c_str());
            ESP_LOGI(TAG, "Rolling back to fallback network config for future connections");
            config.rollback_wifi_config();
        }
    }
    ESP_LOGI(TAG, "Connected to %s", config.wifi.ssid.c_str());

    ESP_LOGI(TAG, "Configuring MQTT client");
    mqtt::BrokerConfiguration mqtt_broker{
            .address = {mqtt::URI{config.mqtt.broker_url}},
            .security =  mqtt::Insecure{}
    };
    mqtt::ClientCredentials mqtt_client_credentials{};
    mqtt::Configuration mqtt_client_config{};

    static MQTTClient mqtt_client{
            mqtt_broker,
            mqtt_client_credentials,
            mqtt_client_config
    };

    ESP_LOGI(TAG, "Connecting to MQTT broker");
    auto mqtt_connection_status = mqtt_client.wait_for_connection(6);
    if (mqtt_connection_status != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to MQTT broker");
        ESP_LOGW(TAG, "Using fallback MQTT broker config");
        mqtt_client.set_uri(config.mqtt_fallback.broker_url);
        mqtt_client.reconnect();
        mqtt_connection_status = mqtt_client.wait_for_connection(6);

        if (mqtt_connection_status != ESP_OK) {
            ESP_LOGE(TAG, "Failed to connect to fallback MQTT broker");
            ESP_LOGW(TAG, "Rebooting...");
            esp_restart();
        } else {
            ESP_LOGI(TAG, "Connected to fallback MQTT broker");
            ESP_LOGI(TAG, "Rolling back to fallback MQTT broker for future connections");
            config.rollback_mqtt_config();
        }
    }
    ESP_LOGI(TAG, "Connected to MQTT broker");

    ESP_LOGI(TAG, "Starting panel and system tasks");
    TaskContext ctx{
            .inkplate = inkplate,
            .config = config,
            .mqtt = mqtt_client
    };
    std::thread panel_task_thread(panel_task, std::ref(ctx));
    std::thread system_task_thread(system_task, std::ref(ctx));
    ESP_LOGI(TAG, "Panel and system tasks started");

    for (;;) {
        constexpr TickType_t xDelay = 500 / portTICK_PERIOD_MS;
        vTaskDelay(xDelay);
    }
}

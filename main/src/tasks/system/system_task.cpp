#include "system_task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_crt_bundle.h>

#include "utils.h"

static constexpr auto *TAG = "system_task";

static void reboot_handler(const esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "Rebooting...");
    esp_restart();
}

static void perform_ota_update_handler(const esp_mqtt_event_handle_t event) {
    auto data = event->data;
    auto data_len = event->data_len;
    auto total_data_len = event->total_data_len;

    if (data_len != total_data_len) {
        ESP_LOGE(TAG, "OTA update got a chunked response, which is not yet supported");
        return;
    }

    esp_http_client_config_t config = {};
    config.url = data;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &config;

    ESP_LOGI(TAG, "Starting OTA update from %s", data);
    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful, rebooting");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
}

static void config_set_ssid_handler(const esp_mqtt_event_handle_t event, Config& config) {
    auto data = event->data;
    auto data_len = event->data_len;
    auto total_data_len = event->total_data_len;
    esp_err_t err;

    if (data_len != total_data_len) {
        ESP_LOGE(TAG, "Config setter got a chunked response, which is not yet supported");
        return;
    }

    ESP_LOGI(TAG, "Setting SSID to %s", data);
    err = config.set_wifi_ssid(data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ssid: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "SSID set successfully");
    }

    try {
        ESP_LOGI(TAG, "Committing config");
        config.commit();
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to commit config: %s", e.what());
    }
}

static void config_set_password_handler(const esp_mqtt_event_handle_t event, Config& config) {
    auto data = event->data;
    auto data_len = event->data_len;
    auto total_data_len = event->total_data_len;

    if (data_len != total_data_len) {
        ESP_LOGE(TAG, "Config setter got a chunked response, which is not yet supported");
        return;
    }

    try {
        ESP_LOGI(TAG, "Setting password to %s", data);
        config.set_wifi_password(data);
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to set password: %s", e.what());
    }

    try {
        ESP_LOGI(TAG, "Committing config");
        config.commit();
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to commit config: %s", e.what());
    }
}

static void config_set_websocket_url_handler(const esp_mqtt_event_handle_t event, Config& config) {
    auto data = event->data;
    auto data_len = event->data_len;
    auto total_data_len = event->total_data_len;

    if (data_len != total_data_len) {
        ESP_LOGE(TAG, "Config setter got a chunked response, which is not yet supported");
        return;
    }

    try {
        ESP_LOGI(TAG, "Setting websocket url to %s", data);
        config.set_websocket_url(data);
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to set websocket url: %s", e.what());
    }

    try {
        ESP_LOGI(TAG, "Committing config");
        config.commit();
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to commit config: %s", e.what());
    }
}

[[noreturn]]
void system_task(const TaskContext &ctx) {
    using idf::mqtt::Filter;
    auto panel_id = ctx.config.get_panel_id();

    auto update_panel_config_topic = string_format("vsb-eink/%s/config/set", panel_id.c_str());
    ctx.mqtt.register_handler({
        .filter = Filter(update_panel_config_topic),
        .callback = [](const esp_mqtt_event_handle_t event) {}
    });

    auto reboot_panel_topic = string_format("vsb-eink/%s/reboot/set", panel_id.c_str());
    ctx.mqtt.register_handler({
        .filter = Filter(reboot_panel_topic),
        .callback = reboot_handler
    });

    auto update_panel_firmware_topic = string_format("vsb-eink/%s/firmware/update/set", panel_id.c_str());
    ctx.mqtt.register_handler({
        .filter = Filter(update_panel_firmware_topic),
        .callback = perform_ota_update_handler
    });

    for (;;) {
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        auto panel_wifi_rssi_topic = string_format("vsb-eink/%s/wifi/rssi", panel_id.c_str());
        ctx.mqtt.publish<std::string>(panel_wifi_rssi_topic, {.data= std::to_string(ap_info.rssi) });

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
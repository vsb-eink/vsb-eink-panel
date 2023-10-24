#include "system_task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_crt_bundle.h>
#include <cJSON.h>

#include "utils.h"

static constexpr auto *TAG = "system_task";

void reboot_handler(const esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "Rebooting...");
    esp_restart();
}

void perform_ota_update_handler(const esp_mqtt_event_handle_t event) {
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

void publish_system_status(const TaskContext &ctx) {
    using idf::mqtt::Retain;

    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    auto panel_system_status_topic = string_format("vsb-eink/%s/system", ctx.config.panel.panel_id.c_str());

    auto system_status_json = cJSON_CreateObject();

    auto system_status_wifi_json = cJSON_CreateObject();
    cJSON_AddStringToObject(system_status_wifi_json, "ssid", reinterpret_cast<const char *>(ap_info.ssid));
    cJSON_AddNumberToObject(system_status_wifi_json, "rssi", ap_info.rssi);
    cJSON_AddItemToObject(system_status_json, "network", system_status_wifi_json);

    cJSON_AddNumberToObject(system_status_json, "uptime", esp_timer_get_time() / 1000 / 1000);
    cJSON_AddNumberToObject(system_status_json, "freeHeap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(system_status_json, "minFreeHeap", esp_get_minimum_free_heap_size());
    cJSON_AddStringToObject(system_status_json, "firmwareVersion", esp_app_get_description()->version);

    auto system_status_json_str = cJSON_PrintUnformatted(system_status_json);
    ctx.mqtt.publish<std::string>(panel_system_status_topic, { .data=system_status_json_str, .retain = Retain::Retained });

    cJSON_Delete(system_status_json);
    free(system_status_json_str);
}

void update_config_handler(const TaskContext &ctx, const esp_mqtt_event_handle_t event) {
    auto data = event->data;
    auto data_len = event->data_len;
    auto total_data_len = event->total_data_len;

    if (data_len != total_data_len) {
        ESP_LOGE(TAG, "Config update got a chunked response, which is not yet supported");
        return;
    }

    auto config_json = cJSON_Parse(data);
    if (!cJSON_IsObject(config_json)) {
        ESP_LOGE(TAG, "Failed to parse config JSON");
        return;
    }

    auto panel_config_json = cJSON_GetObjectItem(config_json, "panel");
    auto wifi_config_json = cJSON_GetObjectItem(config_json, "wifi");
    auto mqtt_config_json = cJSON_GetObjectItem(config_json, "mqtt");

    // update panel config
    bool panel_config_changed = false;
    if (cJSON_IsObject(panel_config_json)) {
        auto panel_id_json = cJSON_GetObjectItem(panel_config_json, "panel_id");
        auto waveform_json = cJSON_GetObjectItem(panel_config_json, "waveform");

        if (cJSON_IsString(panel_id_json)) {
            ctx.config.set_panel_config({
                .panel_id = panel_id_json->valuestring,
                .waveform = ctx.config.panel.waveform
            });
            panel_config_changed = true;
        }

        if (cJSON_IsNumber(waveform_json)) {
            ctx.config.set_panel_config({
                .panel_id = ctx.config.panel.panel_id,
                .waveform = static_cast<uint8_t>(waveform_json->valueint)
            });
            panel_config_changed = true;
        }
    }

    // update wifi config
    bool wifi_config_changed = false;
    if (cJSON_IsObject(wifi_config_json)) {
        auto wifi_ssid_json = cJSON_GetObjectItem(wifi_config_json, "ssid");
        auto wifi_password_json = cJSON_GetObjectItem(wifi_config_json, "password");

        if (!(cJSON_IsString(wifi_ssid_json) && cJSON_IsString(wifi_password_json))) {
            ESP_LOGE(TAG, "Invalid wifi config JSON, both ssid and password must be valid strings");
            return;
        }

        ctx.config.set_wifi_config({
            .ssid = wifi_ssid_json->valuestring,
            .password = wifi_password_json->valuestring
        });
        wifi_config_changed = true;
    }

    // update mqtt config
    bool mqtt_config_changed = false;
    if (cJSON_IsObject(mqtt_config_json)) {
        auto mqtt_broker_url_json = cJSON_GetObjectItem(mqtt_config_json, "broker_url");

        if (cJSON_IsString(mqtt_broker_url_json)) {
            ctx.config.set_mqtt_config({
                .broker_url = mqtt_broker_url_json->valuestring
            });
            mqtt_config_changed = true;
        }
    }

    // commit changes
    if (panel_config_changed) ESP_ERROR_CHECK_WITHOUT_ABORT(ctx.config.commit_panel_config());
    if (wifi_config_changed) ESP_ERROR_CHECK_WITHOUT_ABORT(ctx.config.commit_wifi_config());
    if (mqtt_config_changed) ESP_ERROR_CHECK_WITHOUT_ABORT(ctx.config.commit_mqtt_config());
}

[[noreturn]]
void system_task(const TaskContext &ctx) {
    using idf::mqtt::Filter;
    using idf::mqtt::Retain;
    auto panel_id = ctx.config.panel.panel_id;

    auto update_panel_config_topic = string_format("vsb-eink/%s/config/set", panel_id.c_str());
    ctx.mqtt.register_handler({
        .filter = Filter(update_panel_config_topic),
        .callback = [&](const esp_mqtt_event_handle_t event) { update_config_handler(ctx, event); }
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

    DebounceTimer system_status_debounce_timer(std::chrono::milliseconds(3000));
    for (;;) {
        if (system_status_debounce_timer.tick()) {
            publish_system_status(ctx);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
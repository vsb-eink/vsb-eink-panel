#include "system_task.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"

#include "events.hpp"
#include "utils.hpp"

static constexpr auto *TAG = "system_task";

static void reboot_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Rebooting...");
    esp_restart();
}

static void perform_ota_update_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<eink_panel_event_data_t *>(event_data);

    if (data->data_len != data->payload_len) {
        ESP_LOGE(TAG, "OTA update got a chunked response, which is not yet supported");
        return;
    }

    esp_http_client_config_t config = {};
    config.url = data->data_ptr;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_https_ota_config_t ota_config = {};
    ota_config.http_config = &config;

    ESP_LOGI(TAG, "Starting OTA update from %s", data->data_ptr);
    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful, rebooting");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
}

static void config_set_ssid_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<eink_panel_event_data_t *>(event_data);
    auto shared = static_cast<system_task_config_t *>(handler_args);
    auto config = shared->config;

    if (data->data_len != data->payload_len) {
        ESP_LOGE(TAG, "Config setter got a chunked response, which is not yet supported");
        return;
    }

    try {
        ESP_LOGI(TAG, "Setting SSID to %s", data->data_ptr);
        config->set_wifi_ssid(data->data_ptr);
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to set ssid: %s", e.what());
    }

    try {
        ESP_LOGI(TAG, "Committing config");
        config->commit();
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to commit config: %s", e.what());
    }
}

static void config_set_password_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<eink_panel_event_data_t *>(event_data);
    auto shared = static_cast<system_task_config_t *>(handler_args);
    auto config = shared->config;

    if (data->data_len != data->payload_len) {
        ESP_LOGE(TAG, "Config setter got a chunked response, which is not yet supported");
        return;
    }

    try {
        ESP_LOGI(TAG, "Setting password to %s", data->data_ptr);
        config->set_wifi_password(data->data_ptr);
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to set password: %s", e.what());
    }

    try {
        ESP_LOGI(TAG, "Committing config");
        config->commit();
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to commit config: %s", e.what());
    }
}

static void config_set_websocket_url_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<eink_panel_event_data_t *>(event_data);
    auto shared = static_cast<system_task_config_t *>(handler_args);
    auto config = shared->config;

    if (data->data_len != data->payload_len) {
        ESP_LOGE(TAG, "Config setter got a chunked response, which is not yet supported");
        return;
    }

    try {
        ESP_LOGI(TAG, "Setting websocket url to %s", data->data_ptr);
        config->set_websocket_url(data->data_ptr);
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to set websocket url: %s", e.what());
    }

    try {
        ESP_LOGI(TAG, "Committing config");
        config->commit();
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Failed to commit config: %s", e.what());
    }
}

[[noreturn]]
void system_task(void *pvParameters) {
    auto shared = static_cast<system_task_config_t *>(pvParameters);

    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::REBOOT), &reboot_handler, shared);
    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::PERFORM_OTA_UPDATE), &perform_ota_update_handler, shared);
    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::CONFIG_SET_SSID), &config_set_ssid_handler, shared);
    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::CONFIG_SET_PASSWORD), &config_set_password_handler, shared);
    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::CONFIG_SET_WEBSOCKET_URL), &config_set_websocket_url_handler, shared);

    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
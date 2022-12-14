#include "nvs.h"
#include "esp_event.h"

#include "inkplate.hpp"

#include "tasks/panel/panel_task.hpp"
#include "tasks/system/system_task.hpp"
#include "tasks/websocket/websocket_task.hpp"

#include "events.hpp"
#include "config.hpp"
#include "utils.hpp"

static constexpr auto* TAG = "main";

ESP_EVENT_DEFINE_BASE(EINK_INTERNAL_EVENT_BASE);

extern "C" void app_main() {
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "Initialized NVS");

    ESP_LOGI(TAG, "Initializing display driver");
    Inkplate inkplate(DisplayMode::INKPLATE_1BIT);
    inkplate.begin();
    ESP_LOGI(TAG, "Initialized display driver");

    ESP_LOGI(TAG, "Initializing persistent config");
    Config configStore;
    ESP_ERROR_CHECK(configStore.init());
    ESP_LOGI(TAG, "Initialized persistent config");

    ESP_LOGI(TAG, "Connecting to wifi");
    auto wifi_ssid = configStore.get_wifi_ssid();
    auto wifi_password = configStore.get_wifi_password();
    inkplate.joinAP(wifi_ssid.c_str(), wifi_password.c_str());
    ESP_LOGI(TAG, "Connected to wifi");

    ESP_LOGI(TAG, "Initializing internal event loop");
    esp_event_loop_handle_t eink_internal_event_loop_handle;
    esp_event_loop_args_t eink_internal_event_loop_config = {
            .queue_size = 16,
            .task_name = "eink_internal_event_loop_task",
            .task_priority = 5,
            .task_stack_size = 10000,
            .task_core_id = 1
    };
    esp_event_loop_create(&eink_internal_event_loop_config, &eink_internal_event_loop_handle);
    configASSERT(eink_internal_event_loop_handle);
    ESP_LOGI(TAG, "Initialized internal event loop");

    ESP_LOGI(TAG, "Starting panel task");
    panel_task_config_t panel_task_config = {
            .eink_internal_event_loop_handle = eink_internal_event_loop_handle,
            .config = &configStore,
            .inkplate = &inkplate
    };
    TaskHandle_t panel_task_handle;
    xTaskCreate(&panel_task, "panel_task", 4096, &panel_task_config, 5, &panel_task_handle);
    configASSERT(panel_task_handle);
    ESP_LOGI(TAG, "Started panel task");

    ESP_LOGI(TAG, "Starting system task");
    system_task_config_t system_task_config = {
            .eink_internal_event_loop_handle = eink_internal_event_loop_handle,
            .config = &configStore,
    };
    TaskHandle_t system_task_handle;
    xTaskCreate(&system_task, "system_task", 4096, &system_task_config, 5, &system_task_handle);
    configASSERT(system_task_handle);
    ESP_LOGI(TAG, "Started system task");

    ESP_LOGI(TAG, "Starting websocket task");
    websocket_task_config_t websocket_task_config = {
            .eink_internal_event_loop_handle = eink_internal_event_loop_handle,
            .config = &configStore,
    };
    TaskHandle_t websocket_task_handle;
    xTaskCreate(&websocket_task, "websocket_task", 4096, &websocket_task_config, 5, &websocket_task_handle);
    configASSERT(websocket_task_handle);
    ESP_LOGI(TAG, "Started websocket task");

    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

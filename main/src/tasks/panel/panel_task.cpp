#include "panel_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "../../utils.h"

void panel_task(const TaskContext &ctx) {
    auto panel_id = ctx.config.get_panel_id();

    ctx.mqtt.register_handler({
        .filter{"panel/+/set"},
        .qos = idf::mqtt::QoS::AtLeastOnce,
        .callback = [](const esp_mqtt_event_handle_t event) {
            ESP_LOGI("panel_task", "Got panel message: %.*s", event->data_len, event->data);
        }
    });

    for (;;) {
        for (auto touchpad_id : {PAD1, PAD2, PAD3}) {
            if (ctx.inkplate.readTouchpad(touchpad_id) == 1) {
                ESP_LOGI("panel_task", "Touchpad %d pressed", touchpad_id);

                string_format("eink/%s/touchpad/%d/pressed", panel_id.c_str(), touchpad_id);

                ctx.mqtt.publish<std::string>("eink/", {.data = "1"});
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

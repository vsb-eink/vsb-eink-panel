#include "panel_task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "drivers/inkplate_button.h"
#include "drivers/inkplate_touchpad.h"
#include "utils.h"

void display_1bpp(const TaskContext &ctx, const esp_mqtt_event_handle_t event) {
    // switch to 1 bit mode if not already in it
    if (ctx.inkplate.getDisplayMode() != DisplayMode::INKPLATE_1BIT) {
        ESP_LOGI("display_1bpp", "Switching to 1 bit mode");
        ctx.inkplate.setDisplayMode(DisplayMode::INKPLATE_1BIT);
        ctx.inkplate.clearDisplay();
        ctx.inkplate.display();
    }

    // unpack and draw incoming pixels to the screen
    for (int group_offset = 0; group_offset < event->data_len; group_offset++) {
        auto total_group_offset = event->current_data_offset + group_offset;
        for (int pixel_index_in_group = 0; pixel_index_in_group < 8; pixel_index_in_group++) {
            auto position = get_position_by_index(total_group_offset * 8 + pixel_index_in_group, ctx.inkplate.einkWidth());
            auto color = (event->data[group_offset] >> pixel_index_in_group) & 1;
            ctx.inkplate.drawPixel(position.x, position.y, color);
        }
    }

    // display the screen if we have received all the data
    if (event->current_data_offset + event->data_len == event->total_data_len) {
        ctx.inkplate.display();
    }
}

void display_4bpp(const TaskContext &ctx, const esp_mqtt_event_handle_t event) {
    // switch to 4 bit mode if not already in it
    if (ctx.inkplate.getDisplayMode() != DisplayMode::INKPLATE_3BIT) {
        ESP_LOGI("display_4bpp", "Switching to 4 bit mode");
        ctx.inkplate.setDisplayMode(DisplayMode::INKPLATE_3BIT);
        ctx.inkplate.clearDisplay();
        ctx.inkplate.display();
    }

    // unpack and draw incoming pixels to the screen
    for (int group_offset = 0; group_offset < event->data_len; group_offset++) {
        auto total_group_offset = event->current_data_offset + group_offset;
        for (int pixel_index_in_group = 0; pixel_index_in_group < 2; pixel_index_in_group++) {
            auto position = get_position_by_index(total_group_offset * 2 + pixel_index_in_group, ctx.inkplate.einkWidth());
            auto color = (event->data[group_offset] >> (pixel_index_in_group * 4)) & 0b1111;
            ctx.inkplate.drawPixel(position.x, position.y, color);
        }
    }

    // display the screen if we have received all the data
    if (event->current_data_offset + event->data_len == event->total_data_len) {
        ctx.inkplate.display();
    }
}

void panel_task(const TaskContext &ctx) {
    using idf::mqtt::Filter;
    using idf::mqtt::Message;
    using idf::mqtt::QoS;
    using idf::mqtt::Retain;
    using namespace std::chrono;

    auto panel_id = ctx.config.panel.panel_id;

    auto touchpad = InkplateTouchpad(
        ctx.inkplate,
        [&](const InkplateTouchpadEvent event) {
            auto btn_id = event.pad_id;
            auto btn_action = event.event_type;
            auto btn_action_str = btn_action == InkplateButton::ButtonState::PRESSED ? "pressed" : "released";

            ESP_LOGI("panel_task", "Touchpad %d %s", btn_id, btn_action_str);
            auto panel_touchpad_action_topic = string_format("vsb-eink/%s/touchpad/%d", panel_id.c_str(), btn_id);
            ctx.mqtt.publish<std::string>(panel_touchpad_action_topic, {.data=btn_action_str,.retain=Retain::NotRetained});

            auto panel_touchpad_pressed_topic = string_format("vsb-eink/%s/touchpad/%d/%s", panel_id.c_str(), btn_id, btn_action_str);
            ctx.mqtt.publish<std::string>(panel_touchpad_pressed_topic, {.data="",.retain=Retain::NotRetained});
        });

    auto update_panel_display_raw_1bpp_topic = string_format("vsb-eink/%s/display/raw_1bpp/set", panel_id.c_str());
    ctx.mqtt.register_handler({
        .filter = Filter(update_panel_display_raw_1bpp_topic),
        .callback = [&](const esp_mqtt_event_handle_t event) {
            display_1bpp(ctx, event);
        }
    });

    auto update_panel_display_raw_4bpp_topic = string_format("vsb-eink/%s/display/raw_4bpp/set", panel_id.c_str());
    ctx.mqtt.register_handler({
        .filter = Filter(update_panel_display_raw_4bpp_topic),
        .callback = [&](const esp_mqtt_event_handle_t event) {
            display_4bpp(ctx, event);
        }
    });

    DebounceTimer touchpad_debounce_timer(milliseconds(100));
    for (;;) {
        if (touchpad_debounce_timer.tick()) {
            touchpad.update();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

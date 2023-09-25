#include "panel_task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "drivers/inkplate_button.h"
#include "utils.h"

void display_1bpp(const TaskContext &ctx, const esp_mqtt_event_handle_t event) {
    // switch to 1 bit mode if not already in it
    if (ctx.inkplate.getDisplayMode() != DisplayMode::INKPLATE_1BIT) {
        ctx.inkplate.setDisplayMode(DisplayMode::INKPLATE_1BIT);
        ctx.inkplate.clearDisplay();
        ctx.inkplate.display();
    }

    // unpack and draw incoming pixels to the screen
    for (int group_offset = event->current_data_offset; group_offset < (event->current_data_offset + event->data_len); group_offset++) {
        for (int pixel_index_in_group = 0; pixel_index_in_group < 8; pixel_index_in_group++) {
            auto position = get_position_by_index(group_offset * 8 + pixel_index_in_group, ctx.inkplate.einkWidth());
            ctx.inkplate.drawPixel(position.x, position.y, (event->data[group_offset] >> pixel_index_in_group) & 1);
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
        ctx.inkplate.setDisplayMode(DisplayMode::INKPLATE_3BIT);
        ctx.inkplate.clearDisplay();
        ctx.inkplate.display();
    }

    // unpack and draw incoming pixels to the screen
    for (int group_offset = event->current_data_offset; group_offset < (event->current_data_offset + event->data_len); group_offset++) {
        for (int pixel_index_in_group = 0; pixel_index_in_group < 2; pixel_index_in_group++) {
            auto position = get_position_by_index(group_offset * 2 + pixel_index_in_group, ctx.inkplate.einkWidth());
            ctx.inkplate.drawPixel(position.x, position.y, (event->data[group_offset] >> (pixel_index_in_group * 4)) & 0b1111);
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

    auto panel_id = ctx.config.get_panel_id();

    auto touchpad_btn_1 = InkplateButton(
        [&]() {
            ESP_LOGI("panel_task", "Touchpad 1 pressed");
            auto panel_button_pressed_topic = string_format("vsb-eink/%s/button/1/pressed", panel_id.c_str());
            ctx.mqtt.publish<std::string>(panel_button_pressed_topic, {});
            },
        [&]() {
            ESP_LOGI("panel_task", "Touchpad 1 released");
            auto panel_button_pressed_topic = string_format("vsb-eink/%s/button/1/pressed", panel_id.c_str());
            ctx.mqtt.publish<std::string>(panel_button_pressed_topic, {});
        }
    );

    auto touchpad_btn_2 = InkplateButton(
            [&]() {
                ESP_LOGI("panel_task", "Touchpad 2 pressed");
                auto panel_button_pressed_topic = string_format("vsb-eink/%s/button/2/pressed", panel_id.c_str());
                ctx.mqtt.publish<std::string>(panel_button_pressed_topic, {});
            },
            [&]() {
                ESP_LOGI("panel_task", "Touchpad 2 released");
                auto panel_button_pressed_topic = string_format("vsb-eink/%s/button/2/pressed", panel_id.c_str());
                ctx.mqtt.publish<std::string>(panel_button_pressed_topic, {});
            }
    );

    auto touchpad_btn_3 = InkplateButton(
            [&]() {
                ESP_LOGI("panel_task", "Touchpad 3 pressed");
                auto panel_button_pressed_topic = string_format("vsb-eink/%s/button/3/pressed", panel_id.c_str());
                ctx.mqtt.publish<std::string>(panel_button_pressed_topic, {});
            },
            [&]() {
                ESP_LOGI("panel_task", "Touchpad 3 released");
                auto panel_button_pressed_topic = string_format("vsb-eink/%s/button/3/pressed", panel_id.c_str());
                ctx.mqtt.publish<std::string>(panel_button_pressed_topic, {});
            }
    );

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

    for (;;) {
        touchpad_btn_1.update(ctx.inkplate.readTouchpad(PAD1));
        touchpad_btn_2.update(ctx.inkplate.readTouchpad(PAD2));
        touchpad_btn_3.update(ctx.inkplate.readTouchpad(PAD3));

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

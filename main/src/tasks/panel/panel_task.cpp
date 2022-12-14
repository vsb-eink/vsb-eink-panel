#include "panel_task.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"

#include "events.hpp"
#include "utils.hpp"

static constexpr auto *TAG = "panel_task";

static void draw_bitmap_1bit_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<eink_panel_event_data_t *>(event_data);
    auto shared = static_cast<panel_task_config_t *>(handler_args);
    auto inkplate = *shared->inkplate;

    auto data_ptr = (uint8_t*) data->data_ptr;
    auto data_len = data->data_len / sizeof(uint8_t);
    auto payload_offset = data->payload_offset / sizeof(uint8_t);
    auto payload_len = data->payload_len / sizeof(uint8_t);

    inkplate.setDisplayMode(DisplayMode::INKPLATE_1BIT);

    for (unsigned int offset = 0; offset < data_len; ++offset) {
        auto pixel_group = data_ptr[offset];
        size_t pixel_group_offset = (payload_offset + offset) * 8;

        for (unsigned int bit_offset = 0; bit_offset < 8; ++bit_offset) {
            auto pixel = (pixel_group >> (7 - bit_offset)) & 1;
            auto x = (pixel_group_offset + bit_offset) % inkplate.width();
            auto y = (pixel_group_offset + bit_offset) / inkplate.width();
            inkplate.drawPixel(x, y, pixel);
        }
    }

    if (payload_offset + data_len == payload_len) {
        inkplate.display();
    }

    free(data->data_ptr);
}

static void draw_bitmap_3bit_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<eink_panel_event_data_t *>(event_data);
    auto shared = static_cast<panel_task_config_t *>(handler_args);
    auto inkplate = *shared->inkplate;

    auto data_ptr = (uint16_t*) data->data_ptr;
    auto data_len = data->data_len / sizeof(uint16_t);
    auto payload_offset = data->payload_offset / sizeof(uint16_t);
    auto payload_len = data->payload_len / sizeof(uint16_t);

    inkplate.setDisplayMode(DisplayMode::INKPLATE_3BIT);

    for (unsigned int offset = 0; offset < data_len; ++offset) {
        auto pixel_group = data_ptr[offset];
        size_t pixel_group_offset = (payload_offset + offset) * 5;

        uint8_t pixel_color_1 = (0b1110000000000000 & pixel_group) >> 13;
        int16_t pixel_x_1 = pixel_group_offset % inkplate.width();
        int16_t pixel_y_1 = pixel_group_offset / inkplate.width();

        uint8_t pixel_color_2 = (0b0001110000000000 & pixel_group) >> 10;
        int16_t pixel_x_2 = (pixel_group_offset + 1) % inkplate.width();
        int16_t pixel_y_2 = (pixel_group_offset + 1) / inkplate.width();

        uint8_t pixel_color_3 = (0b0000001110000000 & pixel_group) >> 7;
        int16_t pixel_x_3 = (pixel_group_offset + 2) % inkplate.width();
        int16_t pixel_y_3 = (pixel_group_offset + 2) / inkplate.width();

        uint8_t pixel_color_4 = (0b0000000001110000 & pixel_group) >> 4;
        int16_t pixel_x_4 = (pixel_group_offset + 3) % inkplate.width();
        int16_t pixel_y_4 = (pixel_group_offset + 3) / inkplate.width();

        uint8_t pixel_color_5 = (0b0000000000001110 & pixel_group) >> 1;
        int16_t pixel_x_5 = (pixel_group_offset + 4) % inkplate.width();
        int16_t pixel_y_5 = (pixel_group_offset + 4) / inkplate.width();

        uint8_t parity_bit = 0b0000000000000001 & pixel_group;

        inkplate.drawPixel(pixel_x_1, pixel_y_1, pixel_color_1);
        inkplate.drawPixel(pixel_x_2, pixel_y_2, pixel_color_2);
        inkplate.drawPixel(pixel_x_3, pixel_y_3, pixel_color_3);
        inkplate.drawPixel(pixel_x_4, pixel_y_4, pixel_color_4);
        inkplate.drawPixel(pixel_x_5, pixel_y_5, pixel_color_5);
    }

    if (payload_offset + data_len == payload_len) {
        inkplate.display();
    }

    free(data->data_ptr);
}

[[noreturn]]
void panel_task(void *pvParameters) {
    auto shared = static_cast<panel_task_config_t *>(pvParameters);

    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::DRAW_BITMAP_1BIT), &draw_bitmap_1bit_handler, shared);
    esp_event_handler_register_with(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(EInkPanelEvent::DRAW_BITMAP_3BIT), &draw_bitmap_3bit_handler, shared);

    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

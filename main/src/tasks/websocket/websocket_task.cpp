#include "websocket_task.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_websocket_client.h"

#include "events.hpp"
#include "utils.hpp"

static constexpr auto *TAG = "websocket_task";

struct websocket_message_state {
    websocket_task_config_t *shared;
    EInkPanelEvent message_type;
};

static void websocket_message_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto data = static_cast<esp_websocket_event_data_t *>(event_data);
    auto state = static_cast<websocket_message_state *>(handler_args);
    auto shared = static_cast<websocket_task_config_t *>(state->shared);

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Websocket connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Websocket disconnected");
            break;
        case WEBSOCKET_EVENT_DATA: {
            if (data->op_code == 1) {
                ESP_LOGW(TAG, "Text received: %s", data->data_ptr);
            }

            if (data->op_code == 0x08 && data->data_len == 2) {
                ESP_LOGW(TAG, "Received closed message with code=%d", 256*data->data_ptr[0] + data->data_ptr[1]);
                return;
            }

            if (data->op_code == 10) {
                ESP_LOGW(TAG, "Received ping message");
                return;
            }

            if (data->op_code == 9) {
                ESP_LOGW(TAG, "Received pong message");
                return;
            }

            if (data->op_code != 0 && data->op_code != 1 && data->op_code != 2) {
                ESP_LOGW(TAG, "Received message with an unhandled opcode %d", data->op_code);
                return;
            }

            if (data->data_len == 0) {
                // ESP_LOGW(TAG, "Received empty message");
                return;
            }

            auto is_first_chunk = data->payload_offset == 0;

            if (is_first_chunk) {
                ESP_LOG_BUFFER_HEX(TAG, data->data_ptr, 4);
                state->message_type = static_cast<EInkPanelEvent>(((uint32_t*)data->data_ptr)[0]);
                ESP_LOGI(TAG, "Received message of type %d", state->message_type);
            }

            auto header_len = sizeof(uint32_t);
            auto data_len = data->data_len - (is_first_chunk ? header_len : 0);
            auto payload_offset = data->payload_offset - (is_first_chunk ? 0 : header_len);
            auto payload_len = data->payload_len - header_len;

            auto data_ptr = (char*)malloc(data_len * sizeof(char));
            memcpy(data_ptr, data->data_ptr + (is_first_chunk ? header_len : 0), data_len);

            eink_panel_event_data_t eink_panel_event_data = {
                .data_ptr = data_ptr,
                .data_len = data_len,
                .payload_len = payload_len,
                .payload_offset = payload_offset,
            };

            esp_event_post_to(shared->eink_internal_event_loop_handle, EINK_INTERNAL_EVENT_BASE, to_underlying(state->message_type), &eink_panel_event_data, sizeof(eink_panel_event_data_t), portMAX_DELAY);

            break;
        }
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "Websocket error %d", data->data_len);
            break;
        default:
            ESP_LOGE(TAG, "Unexpected event %d", event_id);
            break;
    }
}

[[noreturn]]
void websocket_task(void *pvParameters) {
    auto shared = static_cast<websocket_task_config_t *>(pvParameters);

    ESP_LOGI(TAG, "Initializing websocket client");
    auto mac = shared->config->get_panel_id();
    auto authorizationHeader = string_format("Authorization: MAC %s\r\n", mac.c_str());
    auto websocket_url = shared->config->get_websocket_url();
    esp_websocket_client_config_t websocket_cfg = {
            .uri = websocket_url.c_str(),
            .task_stack = 8096,
            .headers = authorizationHeader.c_str()
    };

    websocket_message_state state = {
            .shared = shared,
            .message_type = EInkPanelEvent::NOOP
    };

    auto websocket_client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(websocket_client, WEBSOCKET_EVENT_ANY, websocket_message_handler, &state);

    esp_websocket_client_start(websocket_client);
    ESP_LOGI(TAG, "Initialized websocket client");

    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
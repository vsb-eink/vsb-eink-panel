#pragma once

#include "esp_event.h"

#include "config.hpp"

struct websocket_task_config_t {
    esp_event_loop_handle_t eink_internal_event_loop_handle;
    Config *config;
};

[[noreturn]]
void websocket_task(void *pvParameters);
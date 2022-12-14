#pragma once

#include "esp_event.h"

#include "config.hpp"

struct system_task_config_t {
    esp_event_loop_handle_t eink_internal_event_loop_handle;
    Config *config;
};

[[noreturn]]
void system_task(void *pvParameters);
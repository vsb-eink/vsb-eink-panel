#pragma once

#include "esp_event.h"

#include "config.hpp"
#include "inkplate.hpp"

struct panel_task_config_t {
    esp_event_loop_handle_t eink_internal_event_loop_handle;
    Config *config;
    Inkplate *inkplate;
};

[[noreturn]]
void panel_task(void *pvParameters);
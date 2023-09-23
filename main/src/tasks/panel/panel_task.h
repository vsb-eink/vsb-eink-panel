#pragma once

#include "esp_event.h"
#include "tasks/common.h"

[[noreturn]]
void panel_task(const TaskContext& ctx);
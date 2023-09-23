#pragma once

#include <inkplate.hpp>
#include <thread>

#include "../config.h"
#include "eink_mqtt.h"

struct TaskContext {
    Inkplate &inkplate;
    Config &config;
    MQTTClient &mqtt;
};

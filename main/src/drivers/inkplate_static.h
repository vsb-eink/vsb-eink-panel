#pragma once

#include "inkplate.hpp"

class InkplateStatic : public Inkplate {
private:
    InkplateStatic(DisplayMode mode, bool init_nvs);
public:
    InkplateStatic() = delete;
    InkplateStatic(InkplateStatic const&) = delete;
    void operator=(InkplateStatic const&) = delete;

    static InkplateStatic& getInstance();
};

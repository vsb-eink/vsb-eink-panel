#pragma once

#include <vector>
#include <functional>

#include <inkplate.hpp>

#include "inkplate_button.h"

class InkplateTouchpad {
public:
    InkplateTouchpad(Inkplate &inkplate, const std::function<void(const int pad_id)> &on_press, const std::function<void(const int pad_id)> &on_release);
    void update();
private:
    Inkplate &inkplate;
    std::vector<InkplateButton> buttons;
    std::function<void(const int pad_id)> on_press;
    std::function<void(const int pad_id)> on_release;
};
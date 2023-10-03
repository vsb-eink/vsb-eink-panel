#pragma once

#include <vector>
#include <functional>

#include <inkplate.hpp>

#include "inkplate_button.h"

struct InkplateTouchpadEvent {
    const int pad_id;
    const InkplateButton::ButtonState event_type;
};

class InkplateTouchpad {
public:
    InkplateTouchpad(Inkplate &inkplate, const std::function<void(const InkplateTouchpadEvent event)> &on_event);
    std::optional<InkplateTouchpadEvent> update();
private:
    Inkplate &inkplate;
    std::vector<InkplateButton> buttons;
    std::function<void(const InkplateTouchpadEvent event)> on_event;
};
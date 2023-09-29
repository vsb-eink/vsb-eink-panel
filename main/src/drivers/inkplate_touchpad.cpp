#include "inkplate_touchpad.h"

InkplateTouchpad::InkplateTouchpad(Inkplate &inkplate, const std::function<void(const int pad_id)> &on_press, const std::function<void(const int pad_id)> &on_release) :
        inkplate{inkplate},
        on_press{on_press},
        on_release{on_release} {
    for (const auto pad_id: {PAD1, PAD2, PAD3}) {
        buttons.emplace_back(
           pad_id,
           [&]() {
                this->on_press(pad_id);
            }, [&]() {
                this->on_release(pad_id);
           }
        );
    }
}

void InkplateTouchpad::update() {
    for (auto& btn : buttons) {
        auto state = inkplate.readTouchpad(btn.id);
        btn.update(state);
    }
}
#include "inkplate_touchpad.h"

InkplateTouchpad::InkplateTouchpad(Inkplate &inkplate, const std::function<void(const InkplateTouchpadEvent event)> &on_event) :
        inkplate{inkplate},
        on_event{on_event} {
    for (const auto pad_id: {PAD1, PAD2, PAD3}) {
        buttons.emplace_back(
           pad_id
        );
    }
}

std::optional<InkplateTouchpadEvent> InkplateTouchpad::update() {
    for (auto& btn : buttons) {
        auto state = inkplate.readTouchpad(btn.id);

        auto button_event = btn.update(state);
        if (!button_event) continue;

        auto event = InkplateTouchpadEvent{
            .pad_id = btn.id,
            .event_type = button_event.value(),
        };

        this->on_event(event);
        return event;
    }
    return std::nullopt;
}
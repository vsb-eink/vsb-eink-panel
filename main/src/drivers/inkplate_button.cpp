#include "inkplate_button.h"

InkplateButton::InkplateButton(const int id):
        id{id},
        state{ButtonState::RELEASED} {}

std::optional<InkplateButton::ButtonState> InkplateButton::update(const uint8_t state_raw) {
    auto old_state = this->state;
    auto incoming_state = state_raw == 1 ? ButtonState::PRESSED : ButtonState::RELEASED;

    if (old_state == incoming_state) {
        return std::nullopt;
    }

    this->state = incoming_state;

    if (old_state == ButtonState::RELEASED && incoming_state == ButtonState::PRESSED) {
        return incoming_state;
    } else if (old_state == ButtonState::PRESSED && incoming_state == ButtonState::RELEASED) {
        return incoming_state;
    }

    return std::nullopt;
}
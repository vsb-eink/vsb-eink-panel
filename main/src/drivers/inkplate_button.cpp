#include "inkplate_button.h"

#include <esp_log.h>

void InkplateButton::update(const uint8_t state_raw, const std::chrono::milliseconds debounce_delay) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto time_since_last_change = duration_cast<milliseconds>(now - this->last_change);
    if (time_since_last_change < debounce_delay) {
        return;
    }

    auto old_state = this->state;
    auto incoming_state = state_raw == 1 ? ButtonState::PRESSED : ButtonState::RELEASED;

    if (old_state == incoming_state) {
        return;
    }

    if (old_state == ButtonState::RELEASED && incoming_state == ButtonState::PRESSED) {
        this->on_press();
    } else if (old_state == ButtonState::PRESSED && incoming_state == ButtonState::RELEASED) {
        this->on_release();
    }

    this->state = incoming_state;
}
#pragma once

#include <functional>
#include <optional>

class InkplateButton {
public:
    enum class ButtonState {
        RELEASED = 0,
        PRESSED = 1,
    };

    explicit InkplateButton(int id);
    friend class InkplateTouchpad;

    std::optional<ButtonState> update(uint8_t state_raw);
protected:
    const int id;
    ButtonState state;
};
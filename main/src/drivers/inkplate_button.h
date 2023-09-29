#pragma once

#include <functional>
#include <chrono>

class InkplateButton {
public:
    const int id;

    enum class ButtonState {
        RELEASED = 0,
        PRESSED = 1,
    };

    InkplateButton(const int id, const std::function<void()>& on_press, const std::function<void()>& on_release) :
            id{id},
            state{ButtonState::RELEASED},
            last_change{std::chrono::system_clock::now()},
            on_press{on_press},
            on_release{on_release} {}

    void update(uint8_t state_raw, std::chrono::milliseconds debounce_delay = std::chrono::milliseconds(100));
private:
    ButtonState state;
    std::chrono::time_point<std::chrono::system_clock> last_change;
    std::function<void()> on_press;
    std::function<void()> on_release;
};
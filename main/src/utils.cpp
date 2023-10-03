#include "utils.h"

Position2D get_position_by_index(const int index, const int width) {
    return {
            .x = index % width,
            .y = index / width
    };
}

DebounceTimer::DebounceTimer(const std::chrono::milliseconds debounce_time_ms, const std::function<void()> &callback):
        debounce_time_ms{debounce_time_ms},
        callback{callback},
        last_state_change{std::chrono::steady_clock::now()} {}

bool DebounceTimer::tick() {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_state_change = now - this->last_state_change;

    if (time_since_last_state_change > this->debounce_time_ms) {
        this->last_state_change = now;
        this->callback();
        return true;
    }

    return false;
}
#include "utils.h"

Position2D get_position_by_index(const int index, const int width) {
    return {
            .x = index % width,
            .y = index / width
    };
}
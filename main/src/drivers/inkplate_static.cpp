#include "inkplate_static.h"

InkplateStatic::InkplateStatic(DisplayMode mode, bool init_nvs) : Adafruit_GFX(e_ink.get_width(), e_ink.get_width()), Inkplate(mode, init_nvs) {}

InkplateStatic& InkplateStatic::getInstance() {
    static InkplateStatic inkplate(DisplayMode::INKPLATE_3BIT, true);
    return inkplate;
}

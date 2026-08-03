#include "visualizers.h"
#include "led_manager.h"
#include "diyhue_manager.h"
#include <FastLED.h>

void drawDiyHueColor() {
    if (DiyHueManager::isOn()) {
        uint8_t r, g, b;
        DiyHueManager::getRgbColor(r, g, b);
        fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    } else {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
}

#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

enum VisualizerMode {
    MODE_DIAGNOSTIC_HEART = 0,
    MODE_SPECTRUM_LINEAR,
    MODE_SPECTRUM_SYMMETRIC,
    MODE_VU_METER,
    MODE_BASS_PULSE,
    MODE_SOUND_RIPPLES,
    MODE_NOISE,
    MODE_RAINBOW_WAVE,
    MODE_FIRE_PORTAL,
    MODE_DIGITAL_RAIN,
    MODE_PULSING_TUNNEL,
    MODE_COUNT // Keeps track of total modes
};

namespace LEDManager {
    // Initialize the FastLED setup, grid dimensions, and safe brightness level
    void init();

    // Calculate and draw the active animation frame on the LED matrix
    void update();

    // Set a specific animation mode
    void setMode(VisualizerMode mode);

    // Switch to the next available animation mode
    void nextMode();

    // Get the name of the active mode for debug logging and OLED display
    const char* getModeName(VisualizerMode mode);

    // Get the currently active mode
    VisualizerMode getActiveMode();

    // Set global brightness (0-255)
    void setBrightness(uint8_t brightness);

    // Get current global brightness
    uint8_t getBrightness();

    // Set auto-cycle enabled/disabled
    void setAutoCycle(bool enabled);

    // Get auto-cycle status
    bool getAutoCycle();
}

#endif // LED_MANAGER_H

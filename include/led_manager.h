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
}

#endif // LED_MANAGER_H

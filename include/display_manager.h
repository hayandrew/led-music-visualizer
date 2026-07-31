#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

namespace DisplayManager {
    // Initialize the SSD1306 OLED display using custom I2C pins
    void init();

    // Redraw the screen contents (menu & real-time volume diagnostics) at non-blocking intervals
    void update();
}

#endif // DISPLAY_MANAGER_H

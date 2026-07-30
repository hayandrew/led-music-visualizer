#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// LED Matrix Geometry
#define MATRIX_WIDTH 15
#define MATRIX_HEIGHT 17
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define SERPENTINE true

// Hardware Pins
#define LED_PIN 2

// Wi-Fi AP Settings
#define AP_SSID "ESP32C3-Visualizer"
#define AP_PASSWORD "12345678"

#endif // CONFIG_H

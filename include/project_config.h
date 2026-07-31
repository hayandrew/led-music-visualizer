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

// I2S Microphone Configuration (INMP441)
#define I2S_SCK_PIN 8
#define I2S_WS_PIN  3
#define I2S_SD_PIN  4

// Audio Sampling Configuration
#define I2S_SAMPLE_RATE 16000  // 16kHz sampling rate
#define I2S_BUFFER_SIZE 256    // Number of samples per DMA read block

// Wi-Fi AP Settings
#define AP_SSID "ESP32C3-Visualizer"
#define AP_PASSWORD "12345678"

#endif // CONFIG_H

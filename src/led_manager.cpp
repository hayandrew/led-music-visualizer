#include "led_manager.h"
#include "project_config.h"
#include "visualizers.h"
#include <FastLED.h>

CRGB leds[NUM_LEDS];
static SourceMode currentSource = SOURCE_SOUND;
static VisualizerMode currentMode = MODE_RAINBOW_WAVE;
static bool autoCycleEnabled = true;

// Serpentine Index Mapping
uint16_t getLEDIndex(uint8_t x, uint8_t y) {
    if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT) return 0;
    if (x % 2 == 0) {
        // Even columns (0, 2, 4...) run bottom-to-top
        return x * MATRIX_HEIGHT + y;
    } else {
        // Odd columns (1, 3, 5...) run top-to-bottom
        return x * MATRIX_HEIGHT + (MATRIX_HEIGHT - 1 - y);
    }
}

namespace LEDManager {

void init() {
    // Initialize FastLED with GRB color order on LED_PIN
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    // Set a safe global brightness level (102 out of 255, approx 40% brightness)
    // to prevent drawing excessive current from a USB power source
    FastLED.setBrightness(102);
    FastLED.clear();
    FastLED.show();
    
    Serial.println("[LED] LED Manager Initialized. FastLED Ready.");
}

void update() {
    if (currentSource == SOURCE_WIFI) {
        drawDiyHueColor();
    } else {
        // Choose rendering function based on active mode
        switch (currentMode) {
            case MODE_RAINBOW_WAVE:
                drawRainbowWave();
                break;
            case MODE_SOUND_RIPPLES:
                drawSoundRipples();
                break;
            case MODE_LAVA_LAMP:
                drawLavaLamp();
                break;
            case MODE_DIGITAL_RAIN:
                drawDigitalRain();
                break;
            case MODE_AUDIO_PARTICLES:
                drawAudioParticles();
                break;
            case MODE_SPECTRUM_LINEAR:
                drawSpectrumLinear();
                break;
            case MODE_PULSING_TUNNEL:
                drawPulsingTunnel();
                break;
            case MODE_MARIO_RUN:
                drawMarioRun();
                break;
            default:
                FastLED.clear();
                break;
        }
    }
    
    FastLED.show();
}

void setMode(VisualizerMode mode) {
    if (mode < MODE_COUNT) {
        currentMode = mode;
        FastLED.clear();
        Serial.printf("[LED] Mode changed to: %s\n", getModeName(currentMode));
    }
}

SourceMode getSource() {
    return currentSource;
}

void setSource(SourceMode source) {
    currentSource = source;
    FastLED.clear();
    Serial.printf("[LED] Source changed to: %s\n", getSourceName(currentSource));
}

const char* getSourceName(SourceMode source) {
    switch (source) {
        case SOURCE_SOUND: return "Sound";
        case SOURCE_WIFI:  return "WiFi";
        default:           return "Unknown";
    }
}

void nextMode() {
    uint8_t next = (uint8_t)currentMode + 1;
    if (next >= MODE_COUNT) {
        next = 0;
    }
    setMode((VisualizerMode)next);
}

const char* getModeName(VisualizerMode mode) {
    switch (mode) {
        case MODE_RAINBOW_WAVE:       return "Rainbow Wave";
        case MODE_SOUND_RIPPLES:      return "Sound Ripples";
        case MODE_LAVA_LAMP:          return "Lava Lamp";
        case MODE_DIGITAL_RAIN:       return "Digital Rain";
        case MODE_AUDIO_PARTICLES:    return "Particle";
        case MODE_SPECTRUM_LINEAR:    return "Linear Spectrum";
        case MODE_PULSING_TUNNEL:     return "Pulsing Tunnel";
        case MODE_MARIO_RUN:          return "Super Mario Run";
        // case MODE_DIAGNOSTIC_HEART:   return "Diagnostic Heart";
        // case MODE_NOISE:              return "Ambient Noise";
        // case MODE_FIRE_PORTAL:        return "Fire Portal";
        // case MODE_SUBSCRIBE:          return "Subscribe";
        default:                      return "Unknown";
    }
}

VisualizerMode getActiveMode() {
    return currentMode;
}

void setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
}

uint8_t getBrightness() {
    return FastLED.getBrightness();
}

void setAutoCycle(bool enabled) {
    autoCycleEnabled = enabled;
}

bool getAutoCycle() {
    return autoCycleEnabled;
}

} // namespace LEDManager

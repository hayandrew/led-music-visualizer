#include <FastLED.h>
#include "config.h"
#include "led_diagnostics.h"

static CRGB leds[NUM_LEDS];
static unsigned long lastPatternChange = 0;
static int currentPattern = 0;
static int chaseIndex = 0;
static unsigned long lastChaseStep = 0;

namespace LEDDiagnostics {

// Helper to translate 2D (x, y) coordinates to 1D FastLED index for serpentine layout
// Assumes column-major serpentine layout (vertical columns of 17 LEDs)
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

void init() {
    // Initialize FastLED with GRB color order on LED_PIN
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    // Set a safe global brightness level (40 out of 255, approx 15% brightness)
    // to prevent drawing excessive current from a USB power source
    FastLED.setBrightness(40);
    FastLED.clear();
    FastLED.show();
    
    Serial.println("[LED] FastLED Initialized in Serpentine Mode.");
}

void update() {
    unsigned long now = millis();
    
    // Switch diagnostic mode every 6 seconds
    if (now - lastPatternChange >= 6000) {
        currentPattern = (currentPattern + 1) % 7;
        lastPatternChange = now;
        FastLED.clear();
        chaseIndex = 0;
        Serial.printf("[LED] Switched to Diagnostic Pattern %d\n", currentPattern);
    }

    switch (currentPattern) {
        case 0: // 2D Rainbow Wave
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                    // Create a diagonal shifting rainbow based on coordinates and time
                    uint8_t hue = (x * 12) + (y * 8) + (now / 15);
                    leds[getLEDIndex(x, y)] = CHSV(hue, 255, 255);
                }
            }
            break;

        case 1: { // Math-modeled Pulsating Heartbeat
            // Standard heartbeat has a double beat: thump-thump, then a pause.
            // Cycle duration: 1000ms
            unsigned long cycleMs = now % 1000;
            float pulseScale = 1.0f;
            
            if (cycleMs < 120) {
                // First thump: rapid expand
                float t = cycleMs / 120.0f;
                pulseScale = 0.9f + 0.35f * sin(t * HALF_PI);
            } else if (cycleMs < 240) {
                // First thump relaxation
                float t = (cycleMs - 120) / 120.0f;
                pulseScale = 1.25f - 0.25f * sin(t * HALF_PI);
            } else if (cycleMs < 360) {
                // Second thump: smaller rapid expand
                float t = (cycleMs - 240) / 120.0f;
                pulseScale = 1.0f + 0.18f * sin(t * HALF_PI);
            } else if (cycleMs < 480) {
                // Second thump relaxation
                float t = (cycleMs - 360) / 120.0f;
                pulseScale = 1.18f - 0.28f * sin(t * HALF_PI);
            } else {
                // Resting phase: slight breathing
                float t = (cycleMs - 480) / 520.0f;
                pulseScale = 0.9f - 0.05f * sin(t * PI);
            }

            // Draw on matrix
            float cx = (MATRIX_WIDTH - 1) / 2.0f;     // Center X: 7.0
            float cy = (MATRIX_HEIGHT - 1) / 2.0f + 1; // Center Y: 9.0 (slightly elevated for heart shape geometry)

            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                    // Map coordinate to heart space, scaling size with pulseScale
                    float dx = (x - cx) / (4.5f * pulseScale);
                    float dy = (y - cy) / (5.0f * pulseScale);
                    
                    // Heart algebraic formula: (x^2 + y^2 - 1)^3 - x^2 * y^3 <= 0
                    float a = dx * dx + dy * dy - 1.0f;
                    float heartVal = a * a * a - dx * dx * dy * dy * dy;

                    uint16_t idx = getLEDIndex(x, y);
                    if (heartVal <= 0.0f) {
                        // Pulsating Crimson/Red for the heart body
                        leds[idx] = CRGB(220, 0, 30);
                    } else {
                        // Very faint, breathing purple/blue glow in the background
                        float bgIntensity = 3.0f + 5.0f * (pulseScale - 0.85f);
                        leds[idx] = CRGB(bgIntensity * 0.4f, 0, bgIntensity * 0.8f);
                    }
                }
            }
            break;
        }

        case 2: // Solid Red (Power check)
            fill_solid(leds, NUM_LEDS, CRGB::Red);
            break;
            
        case 3: // Solid Green (Power check)
            fill_solid(leds, NUM_LEDS, CRGB::Green);
            break;
            
        case 4: // Solid Blue (Power check)
            fill_solid(leds, NUM_LEDS, CRGB::Blue);
            break;
            
        case 5: // Solid White (Low Brightness check)
            fill_solid(leds, NUM_LEDS, CRGB::White);
            break;
            
        case 6: // Serpentine Index Chase (Wiring check)
            if (now - lastChaseStep >= 40) {
                lastChaseStep = now;
                leds[chaseIndex] = CRGB::Black;
                chaseIndex = (chaseIndex + 1) % NUM_LEDS;
                leds[chaseIndex] = CRGB::White;
                leds[0] = CRGB::Red; // Keep LED 0 red as reference
            }
            break;
    }
    
    FastLED.show();
}

} // namespace LEDDiagnostics

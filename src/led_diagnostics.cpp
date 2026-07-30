#include <FastLED.h>
#include "config.h"
#include "led_diagnostics.h"

static CRGB leds[NUM_LEDS];
static unsigned long lastPatternChange = 0;
static int currentPattern = 0;
static int chaseIndex = 0;
static unsigned long lastChaseStep = 0;

namespace LEDDiagnostics {

void init() {
    // Initialize FastLED with GRB color order on LED_PIN
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    // Set a safe brightness level (40 out of 255, approx 15% brightness)
    // to prevent drawing excessive current from a USB power source
    FastLED.setBrightness(40);
    FastLED.clear();
    FastLED.show();
    
    Serial.println("[LED] FastLED Initialized on GPIO 2.");
}

void update() {
    unsigned long now = millis();
    
    // Switch diagnostic mode every 5 seconds
    if (now - lastPatternChange >= 5000) {
        currentPattern = (currentPattern + 1) % 5;
        lastPatternChange = now;
        FastLED.clear();
        chaseIndex = 0;
        Serial.printf("[LED] Switched to Diagnostic Pattern %d\n", currentPattern);
    }

    switch (currentPattern) {
        case 0: // Solid Red
            fill_solid(leds, NUM_LEDS, CRGB::Red);
            break;
            
        case 1: // Solid Green
            fill_solid(leds, NUM_LEDS, CRGB::Green);
            break;
            
        case 2: // Solid Blue
            fill_solid(leds, NUM_LEDS, CRGB::Blue);
            break;
            
        case 3: // Solid White (Low Brightness)
            fill_solid(leds, NUM_LEDS, CRGB::White);
            break;
            
        case 4: // Serpentine Chase
            if (now - lastChaseStep >= 40) { // 25 steps per second
                lastChaseStep = now;
                
                // Clear the chase pixel from the previous step
                leds[chaseIndex] = CRGB::Black;
                
                // Increment index
                chaseIndex = (chaseIndex + 1) % NUM_LEDS;
                
                // Highlight the active chase pixel
                leds[chaseIndex] = CRGB::White;
                
                // Mark the starting pixel (LED 0) in Red to verify matrix origin
                leds[0] = CRGB::Red;
            }
            break;
    }
    
    FastLED.show();
}

} // namespace LEDDiagnostics

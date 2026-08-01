#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

void drawNoise() {
    float env = AudioProcessor::getVolumeEnvelope();
    
    // Cycle through randomly chosen color schemes smoothly
    static uint8_t currentBaseHue = random8(); // Initialize with a random color
    static uint8_t targetBaseHue = random8();
    static uint32_t lastTransitionTime = 0;
    
    uint32_t now = millis();
    if (now - lastTransitionTime > 7000) { // Choose a new random color scheme every 7 seconds
        targetBaseHue = random8();
        lastTransitionTime = now;
    }
    
    // Smoothly interpolate currentBaseHue towards targetBaseHue along the shortest path
    uint8_t diff = targetBaseHue - currentBaseHue;
    if (diff > 0) {
        if (diff < 128) {
            currentBaseHue++;
        } else {
            currentBaseHue--;
        }
    }
    uint8_t baseHue = currentBaseHue;
    
    // Modulate space translation speed based on volume (Less sensitive: env / 3500.0f)
    float speed = 1.0f + (env / 3500.0f) * 4.0f;
    static float floatZ = 0;
    floatZ += speed;
    uint32_t zDist = (uint32_t)floatZ;
    
    // Brightness scaling: volumeFactor controls baseline, dynamicBoost adds beat-reactive flare
    float volumeFactor = 0.30f + (env / 8000.0f) * 0.70f;
    volumeFactor = constrain(volumeFactor, 0.30f, 1.0f);
    
    // Beat-driven multiplier instead of flat offset to preserve contrast
    float dynamicBoost = 1.0f + (env / 4000.0f); 
    
    // Shift color spectrum based on volume (Less sensitive: env / 35.0f)
    uint8_t hueShift = constrain((int)(env / 35.0f), 0, 60);
    
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            // Generate Simplex noise: x-scale, y-scale, z-distance
            uint8_t noiseVal = inoise8(x * 35, y * 35, zDist);
            
            // Enhance contrast: push values below 128 lower and above 128 higher
            int contrastVal = 128 + ((int)noiseVal - 128) * 2.2f;
            uint8_t highContrastNoise = constrain(contrastVal, 0, 255);
            
            // Map the noise value to a transitioning hue around the cycling base color scheme
            uint8_t hue = baseHue + (highContrastNoise / 4) + hueShift;
            uint8_t sat = 255 - (highContrastNoise / 8);
            uint8_t baseBri = dim8_raw(highContrastNoise);
            
            // Multiply by volumeFactor and dynamicBoost to keep darks dark and make lights flash
            uint16_t scaledBri = (uint16_t)(baseBri * volumeFactor * dynamicBoost);
            if (scaledBri > 255) scaledBri = 255;
            uint8_t bri = (uint8_t)scaledBri;
            
            leds[getLEDIndex(x, y)] = CHSV(hue, sat, bri);
        }
    }
}

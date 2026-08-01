#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 12. Pulsing Tunnel (Concentric color rings expanding from center, pulsing to the beat)
void drawPulsingTunnel() {
    static float timeOffset = 0;
    float env = AudioProcessor::getVolumeEnvelope();
    
    // Speed up ring expansion with audio volume (faster passing bands)
    float speed = 3.0f + (env / 1500.0f) * 7.0f;
    timeOffset += speed;
    if (timeOffset >= 256.0f) {
        timeOffset -= 256.0f;
    }
    
    float cx = (MATRIX_WIDTH - 1) / 2.0f;
    float cy = (MATRIX_HEIGHT - 1) / 2.0f;
    
    static uint8_t baseHue = 0;
    baseHue += 1;
    
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            
            // Higher spatial frequency for multiple concurrent rings
            // Cast to int first to ensure well-defined negative float casting behavior on ESP32
            uint8_t phase = (uint8_t)(int)(dist * 65.0f - timeOffset);
            uint8_t wave = sin8(phase);
            
            // Shape the wave to get distinct, crisp rings with black gaps (narrower threshold)
            uint8_t bri = 0;
            if (wave > 150) {
                bri = map(wave, 150, 255, 0, 255);
            }
            
            // Entire tunnel pulses in brightness with the beat (scaled by band brightness to preserve black bars)
            uint8_t volBoost = constrain((int)(env / 15.0f), 0, 150);
            uint8_t finalBri = qadd8(bri, (uint8_t)(volBoost * (bri / 255.0f)));
            
            // Spectrum of colors shifting along the radius
            uint8_t hue = baseHue + (uint8_t)(dist * 16);
            
            // Always keep the center hole black
            if (dist < 2.0f) {
                leds[getLEDIndex(x, y)] = CRGB::Black;
            } else if (dist < 3.0f) {
                // Smooth transition at the edge of the black hole
                float fadeFactor = (dist - 2.0f); // 0.0 to 1.0
                uint8_t smoothBri = (uint8_t)(finalBri * fadeFactor);
                leds[getLEDIndex(x, y)] = CHSV(hue, 240, smoothBri);
            } else {
                leds[getLEDIndex(x, y)] = CHSV(hue, 240, finalBri);
            }
        }
    }
}

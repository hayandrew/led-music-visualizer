#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

void drawDiagnosticHeart() {
    unsigned long now = millis();
    unsigned long cycleMs = now % 1000;
    float basePulse = 1.0f;
    
    if (cycleMs < 120) {
        float t = cycleMs / 120.0f;
        basePulse = 0.9f + 0.35f * sin(t * HALF_PI);
    } else if (cycleMs < 240) {
        float t = (cycleMs - 120) / 120.0f;
        basePulse = 1.25f - 0.25f * sin(t * HALF_PI);
    } else if (cycleMs < 360) {
        float t = (cycleMs - 240) / 120.0f;
        basePulse = 1.0f + 0.18f * sin(t * HALF_PI);
    } else if (cycleMs < 480) {
        float t = (cycleMs - 360) / 120.0f;
        basePulse = 1.18f - 0.28f * sin(t * HALF_PI);
    } else {
        float t = (cycleMs - 480) / 520.0f;
        basePulse = 0.9f - 0.05f * sin(t * PI);
    }

    float env = AudioProcessor::getVolumeEnvelope();
    float netEnv = env - 50.0f;
    if (netEnv < 0.0f) netEnv = 0.0f;

    // Significantly increased sensitivity (maxRef from 45000.0f down to 3200.0f)
    float maxRef = 3200.0f;
    float normEnv = netEnv / maxRef;
    if (normEnv > 1.0f) normEnv = 1.0f;
    float soundFactor = sqrt(normEnv);

    // Make the heart scale starting from 0.0 (fully invisible by default) up to 1.1
    float baselineScale = 1.1f * soundFactor;
    float pulseScale = baselineScale * (0.85f + 0.15f * basePulse);

    float cx = (MATRIX_WIDTH - 1) / 2.0f;     
    // Centered vertically (compensated for heart shape center-of-mass)
    float cy = (MATRIX_HEIGHT - 1) / 2.0f - 0.5f; 

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            uint16_t idx = getLEDIndex(x, y);
            
            // If silent or heart scale is too small, render silent background (invisible heart)
            if (pulseScale < 0.08f) {
                leds[idx] = CRGB(1, 0, 2);
                continue;
            }

            float dx = (x - cx) / (4.5f * pulseScale);
            float dy = (y - cy) / (5.0f * pulseScale);
            
            float a = dx * dx + dy * dy - 1.0f;
            float heartVal = a * a * a - dx * dx * dy * dy * dy;

            if (heartVal <= 0.0f) {
                uint8_t redVal = 180 + 75 * soundFactor;
                leds[idx] = CRGB(redVal, 0, 30);
            } else {
                float bgIntensity = 3.0f + 8.0f * (pulseScale - 0.25f);
                if (bgIntensity < 1.0f) bgIntensity = 1.0f;
                leds[idx] = CRGB(bgIntensity * 0.3f, 0, bgIntensity * 0.7f);
            }
        }
    }
}

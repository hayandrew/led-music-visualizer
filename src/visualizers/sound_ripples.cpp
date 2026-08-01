#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 7. Sound Ripples (Transient peaks trigger organic ripples)
struct Ripple {
    float x;
    float y;
    float radius;
    uint8_t hue;
    float life; // 1.0 down to 0.0
    bool active;
};

static Ripple ripples[5] = {0};

void drawSoundRipples() {
    // Slow decay trailing
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].fadeToBlackBy(30);
    }

    float env = AudioProcessor::getVolumeEnvelope();
    float peak = AudioProcessor::getPeakAmplitude();
    
    // Trigger new ripple on sudden volume spike
    static float lastPeak = 0;
    float delta = peak - lastPeak;
    lastPeak = peak;

    if (delta > 1800.0f && env > 800.0f) { // Transient sound trigger
        // Find an inactive ripple slot
        for (int i = 0; i < 5; i++) {
            if (!ripples[i].active) {
                ripples[i].active = true;
                ripples[i].x = random(2, MATRIX_WIDTH - 2);
                ripples[i].y = random(2, MATRIX_HEIGHT - 2);
                ripples[i].radius = 0.5f;
                ripples[i].hue = random8();
                ripples[i].life = 1.0f;
                break;
            }
        }
    }

    // Update and draw ripples
    for (int i = 0; i < 5; i++) {
        if (ripples[i].active) {
            ripples[i].radius += 0.35f;
            ripples[i].life -= 0.04f; // Decay life

            if (ripples[i].life <= 0.0f || ripples[i].radius > 12.0f) {
                ripples[i].active = false;
                continue;
            }

            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                    float dx = (float)x - ripples[i].x;
                    float dy = (float)y - ripples[i].y;
                    float dist = sqrt(dx * dx + dy * dy);

                    if (abs(dist - ripples[i].radius) < 0.8f) {
                        uint16_t idx = getLEDIndex(x, y);
                        uint8_t bri = (1.0f - abs(dist - ripples[i].radius)) * 255.0f * ripples[i].life;
                        leds[idx] += CHSV(ripples[i].hue, 220, bri);
                    }
                }
            }
        }
    }
}

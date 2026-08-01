#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 14. Lava Lamp (Metaballs floating vertically, changing colors slowly, brightness reactive to audio)
void drawLavaLamp() {
    struct Blob {
        float x, y;
        float r;      // Radius
    };
    static Blob blobs[4] = {
        {3.5f, 8.5f, 2.2f},
        {11.5f, 8.5f, 2.8f},
        {7.5f, 8.5f, 2.5f},
        {5.5f, 8.5f, 1.8f}
    };

    static float timeOffset = 0;
    timeOffset += 0.028f; // Smooth speed of floating blobs (increased from 0.012f)

    // Floating animation (sine/cosine waves)
    blobs[0].x = 3.5f + sin(timeOffset * 0.8f) * 1.5f;
    blobs[0].y = 8.5f + cos(timeOffset * 1.1f) * 6.5f;

    blobs[1].x = 11.5f + cos(timeOffset * 0.6f) * 2.0f;
    blobs[1].y = 8.5f + sin(timeOffset * 0.9f) * 6.0f;

    blobs[2].x = 7.5f + sin(timeOffset * 1.2f) * 2.5f;
    blobs[2].y = 8.5f + sin(timeOffset * 0.7f) * 5.5f;

    blobs[3].x = 5.5f + cos(timeOffset * 1.0f) * 1.8f;
    blobs[3].y = 8.5f + cos(timeOffset * 1.3f) * 7.0f;

    // Slow color shift
    static float hueOffset = 0;
    hueOffset += 0.06f; // Increased color shift speed from 0.035f
    uint8_t blobHue = (uint8_t)hueOffset;
    uint8_t bgHue = (uint8_t)(hueOffset + 96); // Contrasting liquid background

    // Keep lava lamp at a constant brightness (no sound-reactivity for brightness)
    float soundMult = 1.0f;

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            float sum = 0.0f;
            for (int i = 0; i < 4; i++) {
                float dx = x - blobs[i].x;
                float dy = y - blobs[i].y;
                float distSq = dx * dx + dy * dy;
                if (distSq < 0.1f) distSq = 0.1f;
                sum += (blobs[i].r * blobs[i].r) / distSq;
            }

            CRGB color;
            if (sum > 1.0f) {
                // Inside a blob: bright and highly sound-reactive
                uint8_t bri = constrain((int)(200 * soundMult * fminf(sum, 2.5f) / 2.5f), 30, 255);
                color = CHSV(blobHue, 240, bri);
            } else if (sum > 0.6f) {
                // Outer glow: smooth blend between background and blob
                float factor = (sum - 0.6f) / 0.4f;
                uint8_t h = lerp8by8(bgHue, blobHue, (uint8_t)(factor * 255));
                uint8_t b = lerp8by8(12, (uint8_t)(200 * soundMult), (uint8_t)(factor * 255));
                color = CHSV(h, 240, b);
            } else {
                // Deep background liquid
                color = CHSV(bgHue, 255, 12);
            }
            leds[getLEDIndex(x, y)] = color;
        }
    }
}

#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 9. Rainbow Wave (Swirling rainbow whose speed is driven by audio intensity)
void drawRainbowWave() {
    static float hueOffset = 0;
    float env = AudioProcessor::getVolumeEnvelope();
    
    // Modulate wave rotation speed based on sound envelope
    float speed = 0.3f + (env / 3000.0f) * 2.0f;
    hueOffset += speed;
    if (hueOffset >= 256.0f) hueOffset -= 256.0f;

    // Saturation and brightness breathing based on volume
    uint8_t sat = 255 - constrain((int)(env / 100.0f), 0, 40);
    uint8_t minBri = 80;
    uint8_t maxBri = 255;
    uint8_t bri = minBri + constrain((int)(env / 25.0f), 0, maxBri - minBri);

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            // Use 3D Simplex Noise (x, y, time) to warp the color gradient into organic swirling clouds
            uint8_t warp = inoise8(x * 50, y * 50, (uint16_t)(hueOffset * 10));
            uint8_t hue = (uint8_t)(x * 5 + y * 5 + warp + hueOffset);
            leds[getLEDIndex(x, y)] = CHSV(hue, sat, bri);
        }
    }
}

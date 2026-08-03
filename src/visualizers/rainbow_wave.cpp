#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 9. Rainbow Wave (Swirling rainbow whose speed is driven by audio intensity)
void drawRainbowWave() {
    static float hueOffset = 0;
    float* bands = AudioProcessor::getFrequencyBands();
    // Use the max of Sub-Bass (band 0) and Bass (band 1) to track the beat
    float bassVal = max(bands[0], bands[1]);
    
    // Modulate wave rotation speed based on the quadratic bass value
    // Minimum slow movement speed of 1.0, scaling up to 9.0 on strong beats
    float speed = 1.0f + (bassVal * bassVal) * 8.0f;
    hueOffset += speed;
    if (hueOffset >= 256.0f) hueOffset -= 256.0f;

    // Keep saturation and brightness at constant full values
    uint8_t sat = 255;
    uint8_t bri = 255;

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            // Use 3D Simplex Noise (x, y, time) to warp the color gradient into organic swirling clouds
            uint8_t warp = inoise8(x * 50, y * 50, (uint16_t)(hueOffset * 10));
            uint8_t hue = (uint8_t)(x * 5 + y * 5 + warp + hueOffset);
            leds[getLEDIndex(x, y)] = CHSV(hue, sat, bri);
        }
    }
}

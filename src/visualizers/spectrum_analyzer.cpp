#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// Helper to draw a single frequency spectrum column with peak fall-off
static float peaks[MATRIX_WIDTH] = {0};
static unsigned long peakTimers[MATRIX_WIDTH] = {0};

void drawSpectrumColumn(uint8_t col, float val) {
    // Map value (0.0 to 1.0) to row height (0 to MATRIX_HEIGHT)
    float targetHeight = val * (float)MATRIX_HEIGHT;
    
    // Smooth the bar height movement
    static float barHeights[MATRIX_WIDTH] = {0};
    barHeights[col] = barHeights[col] * 0.3f + targetHeight * 0.7f;
    int currentHeight = (int)barHeights[col];

    // Peak tracking
    unsigned long now = millis();
    if (barHeights[col] >= peaks[col]) {
        peaks[col] = barHeights[col];
        peakTimers[col] = now;
    } else if (now - peakTimers[col] > 300) { // Hold peak for 300ms
        peaks[col] -= 0.25f; // Fall down slowly
        if (peaks[col] < 0) peaks[col] = 0;
    }

    // Render column
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        uint16_t idx = getLEDIndex(col, y);
        if (y < currentHeight) {
            // Gradient: Green at bottom -> Purple/Pink in middle -> Red/White at top
            uint8_t hue = 140 - (y * (140 / MATRIX_HEIGHT)); // 140 (Green) down to 0 (Red)
            leds[idx] = CHSV(hue, 255, 255);
        } else if (y == (int)peaks[col] && y > 0) {
            // Draw peak dot (light cyan/white)
            leds[idx] = CRGB(180, 255, 255);
        } else {
            // Clear empty space
            leds[idx] = CRGB::Black;
        }
    }
}

// 3. Linear Spectrum Analyzer (Left-to-Right)
void drawSpectrumLinear() {
    float* bands = AudioProcessor::getFrequencyBands();
    
    // Interpolate 7 frequency bands across 15 columns
    for (uint8_t col = 0; col < MATRIX_WIDTH; col++) {
        float bandIdx = (float)col * 6.0f / (float)(MATRIX_WIDTH - 1);
        int lowIdx = (int)floor(bandIdx);
        int highIdx = (int)ceil(bandIdx);
        float t = bandIdx - (float)lowIdx;
        
        float val = bands[lowIdx] * (1.0f - t) + bands[highIdx] * t;
        drawSpectrumColumn(col, val);
    }
}



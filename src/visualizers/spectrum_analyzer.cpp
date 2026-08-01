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

// 4. Symmetrical Spectrum Analyzer (Mirrored from center)
void drawSpectrumSymmetric() {
    float* bands = AudioProcessor::getFrequencyBands();
    float colValues[MATRIX_WIDTH];

    // Mirrored mapping: Bass in center (Col 7), Treble on edges (Col 0 & 14)
    // Map bands 0 to 6 to the columns
    colValues[7] = bands[0]; // Bass in the dead center
    
    for (uint8_t i = 1; i <= 7; i++) {
        // Linearly map the distance to the outer edges to the remaining bands
        float bandIdx = (float)(i - 1) * 6.0f / 6.0f; // Scale distance to 0..6
        int lowIdx = (int)floor(bandIdx);
        int highIdx = (int)ceil(bandIdx);
        float t = bandIdx - (float)lowIdx;
        
        float val = bands[lowIdx] * (1.0f - t) + bands[highIdx] * t;
        colValues[7 - i] = val; // Left side
        colValues[7 + i] = val; // Right side
    }

    for (uint8_t col = 0; col < MATRIX_WIDTH; col++) {
        drawSpectrumColumn(col, colValues[col]);
    }
}

// 5. Stereo VU Meter
static float leftPeak = 0;
static float rightPeak = 0;
static unsigned long leftPeakTime = 0;
static unsigned long rightPeakTime = 0;

void drawVUMeter() {
    FastLED.clear();
    
    // Fetch raw envelope and normalize it
    float env = AudioProcessor::getVolumeEnvelope();
    float netEnv = env - 200.0f;
    if (netEnv < 0.0f) netEnv = 0.0f;
    
    float maxRef = 40000.0f;
    float norm = netEnv / maxRef;
    if (norm > 1.0f) norm = 1.0f;

    // Use lower frequency bands to add a subtle stereo bounce difference
    float* bands = AudioProcessor::getFrequencyBands();
    float leftBias = bands[1] * 0.15f;
    float rightBias = bands[2] * 0.15f;

    float leftVal = norm * 0.85f + leftBias;
    float rightVal = norm * 0.85f + rightBias;
    if (leftVal > 1.0f) leftVal = 1.0f;
    if (rightVal > 1.0f) rightVal = 1.0f;

    float leftHeight = leftVal * MATRIX_HEIGHT;
    float rightHeight = rightVal * MATRIX_HEIGHT;

    unsigned long now = millis();

    // Track left peak
    if (leftHeight >= leftPeak) {
        leftPeak = leftHeight;
        leftPeakTime = now;
    } else if (now - leftPeakTime > 400) {
        leftPeak -= 0.2f;
        if (leftPeak < 0) leftPeak = 0;
    }

    // Track right peak
    if (rightHeight >= rightPeak) {
        rightPeak = rightHeight;
        rightPeakTime = now;
    } else if (now - rightPeakTime > 400) {
        rightPeak -= 0.2f;
        if (rightPeak < 0) rightPeak = 0;
    }

    // Draw Left VU bar (Columns 1, 2, 3, 4)
    for (uint8_t col = 1; col <= 4; col++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            uint16_t idx = getLEDIndex(col, y);
            if (y < (int)leftHeight) {
                // Classic Green -> Yellow -> Red VU gradient
                uint8_t hue = 96 - (y * (96 / MATRIX_HEIGHT)); // Green to Red
                leds[idx] = CHSV(hue, 255, 255);
            } else if (y == (int)leftPeak && y > 0) {
                leds[idx] = CRGB::White;
            }
        }
    }

    // Draw Right VU bar (Columns 10, 11, 12, 13)
    for (uint8_t col = 10; col <= 13; col++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            uint16_t idx = getLEDIndex(col, y);
            if (y < (int)rightHeight) {
                uint8_t hue = 96 - (y * (96 / MATRIX_HEIGHT)); 
                leds[idx] = CHSV(hue, 255, 255);
            } else if (y == (int)rightPeak && y > 0) {
                leds[idx] = CRGB::White;
            }
        }
    }
}

// 6. Bass Pulse (Expanding rings from center triggered by bass frequencies)
static float pulseRadius = 0.0f;
static uint8_t pulseHue = 0;
static float pulseIntensity = 0.0f;

void drawBassPulse() {
    // Slowly dim the existing canvas to leave trailing trails
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].fadeToBlackBy(45);
    }

    float* bands = AudioProcessor::getFrequencyBands();
    float bass = bands[0]; // Sub-bass band (0.0 to 1.0)
    
    // Trigger pulse on bass beat
    if (bass > 0.65f && pulseRadius < 1.5f) {
        pulseRadius = 1.0f;
        pulseHue = random8(); // Random color pulse
        pulseIntensity = bass;
    }

    float cx = (MATRIX_WIDTH - 1) / 2.0f;
    float cy = (MATRIX_HEIGHT - 1) / 2.0f;

    if (pulseRadius > 0.0f) {
        // Expand radius
        pulseRadius += 0.45f;
        // Fade intensity
        pulseIntensity *= 0.90f;

        if (pulseRadius > 14.0f || pulseIntensity < 0.05f) {
            pulseRadius = 0.0f; // Reset
        } else {
            // Draw circle ring
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
                for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
                    float dx = (float)x - cx;
                    float dy = (float)y - cy;
                    float dist = sqrt(dx * dx + dy * dy);

                    // If pixel lies on the ring border, blend it in
                    if (abs(dist - pulseRadius) < 1.0f) {
                        uint16_t idx = getLEDIndex(x, y);
                        // Make brightness scale with pulse intensity
                        uint8_t bri = (1.0f - abs(dist - pulseRadius)) * 255.0f * pulseIntensity;
                        leds[idx] += CHSV(pulseHue, 230, bri);
                    }
                }
            }
        }
    }
    
    // Faint central core glow reacting directly to the bass level
    uint8_t centerVal = bass * 180;
    if (centerVal > 20) {
        leds[getLEDIndex(7, 8)] += CHSV(pulseHue + 32, 255, centerVal);
        leds[getLEDIndex(6, 8)] += CHSV(pulseHue + 32, 255, centerVal / 2);
        leds[getLEDIndex(8, 8)] += CHSV(pulseHue + 32, 255, centerVal / 2);
        leds[getLEDIndex(7, 7)] += CHSV(pulseHue + 32, 255, centerVal / 2);
        leds[getLEDIndex(7, 9)] += CHSV(pulseHue + 32, 255, centerVal / 2);
    }
}

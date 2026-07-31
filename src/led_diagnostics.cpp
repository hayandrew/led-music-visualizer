#include <FastLED.h>
#include "project_config.h"
#include "led_diagnostics.h"
#include "audio_processor.h"

static CRGB leds[NUM_LEDS];

namespace LEDDiagnostics {

// Helper to translate 2D (x, y) coordinates to 1D FastLED index for serpentine layout
// Assumes column-major serpentine layout (vertical columns of 17 LEDs)
uint16_t getLEDIndex(uint8_t x, uint8_t y) {
    if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT) return 0;
    if (x % 2 == 0) {
        // Even columns (0, 2, 4...) run bottom-to-top
        return x * MATRIX_HEIGHT + y;
    } else {
        // Odd columns (1, 3, 5...) run top-to-bottom
        return x * MATRIX_HEIGHT + (MATRIX_HEIGHT - 1 - y);
    }
}

void init() {
    // Initialize FastLED with GRB color order on LED_PIN
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    // Set a safe global brightness level (45 out of 255, approx 17% brightness)
    // to prevent drawing excessive current from a USB power source
    FastLED.setBrightness(45);
    FastLED.clear();
    FastLED.show();
    
    Serial.println("[LED] FastLED Initialized in Serpentine Mode. Running Audio-Reactive Heartbeat.");
}

void update() {
    unsigned long now = millis();
    
    // 1. Calculate time-based heartbeat contraction pulse
    // Standard heartbeat has a double beat: thump-thump, then a pause.
    // Cycle duration: 1000ms
    unsigned long cycleMs = now % 1000;
    float basePulse = 1.0f;
    
    if (cycleMs < 120) {
        // First thump: rapid expand
        float t = cycleMs / 120.0f;
        basePulse = 0.9f + 0.35f * sin(t * HALF_PI);
    } else if (cycleMs < 240) {
        // First thump relaxation
        float t = (cycleMs - 120) / 120.0f;
        basePulse = 1.25f - 0.25f * sin(t * HALF_PI);
    } else if (cycleMs < 360) {
        // Second thump: smaller rapid expand
        float t = (cycleMs - 240) / 120.0f;
        basePulse = 1.0f + 0.18f * sin(t * HALF_PI);
    } else if (cycleMs < 480) {
        // Second thump relaxation
        float t = (cycleMs - 360) / 120.0f;
        basePulse = 1.18f - 0.28f * sin(t * HALF_PI);
    } else {
        // Resting phase: slight breathing
        float t = (cycleMs - 480) / 520.0f;
        basePulse = 0.9f - 0.05f * sin(t * PI);
    }

    // 2. Fetch the microphone volume envelope and calculate sound scale
    float env = AudioProcessor::getVolumeEnvelope();
    
    // Noise floor subtraction (tweak if baseline noise is higher/lower)
    float netEnv = env - 200.0f;
    if (netEnv < 0.0f) netEnv = 0.0f;

    // Normalize against a reference max volume
    float maxRef = 45000.0f;
    float normEnv = netEnv / maxRef;
    if (normEnv > 1.0f) normEnv = 1.0f;
    
    // Apply a square-root compression curve to make the heart highly responsive to voice
    float soundFactor = sqrt(normEnv); // 0.0 to 1.0

    // Map soundFactor to baseline scale: 0.3f (silence) to 1.1f (maximum loudness)
    float baselineScale = 0.3f + 0.8f * soundFactor;

    // Combine base heartbeat pulse with sound-reactive scale
    float pulseScale = baselineScale * (0.85f + 0.15f * basePulse);

    // 3. Render heart shape on matrix using the algebraic formula
    float cx = (MATRIX_WIDTH - 1) / 2.0f;     // Center X: 7.0
    float cy = (MATRIX_HEIGHT - 1) / 2.0f + 1; // Center Y: 9.0 (offset slightly for heart geometry)

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            // Map coordinates to standard heart space, scaling size with pulseScale
            float dx = (x - cx) / (4.5f * pulseScale);
            float dy = (y - cy) / (5.0f * pulseScale);
            
            // Heart algebraic equation: (x^2 + y^2 - 1)^3 - x^2 * y^3 <= 0
            float a = dx * dx + dy * dy - 1.0f;
            float heartVal = a * a * a - dx * dx * dy * dy * dy;

            uint16_t idx = getLEDIndex(x, y);
            if (heartVal <= 0.0f) {
                // Heart color: Pulsating Red
                // Make the red brightness slightly responsive to the sound level for extra visual punch!
                uint8_t redVal = 180 + 75 * soundFactor;
                leds[idx] = CRGB(redVal, 0, 30);
            } else {
                // Faint, breathing background glow
                // Intensity scales up slightly with sound level
                float bgIntensity = 3.0f + 8.0f * (pulseScale - 0.25f);
                if (bgIntensity < 1.0f) bgIntensity = 1.0f;
                leds[idx] = CRGB(bgIntensity * 0.3f, 0, bgIntensity * 0.7f);
            }
        }
    }
    
    FastLED.show();
}

} // namespace LEDDiagnostics

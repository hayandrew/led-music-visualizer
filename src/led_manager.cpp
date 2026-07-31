#include "led_manager.h"
#include "project_config.h"
#include "audio_processor.h"
#include <FastLED.h>

static CRGB leds[NUM_LEDS];
static VisualizerMode currentMode = MODE_SPECTRUM_SYMMETRIC; // Start with mirrored spectrum
static bool autoCycleEnabled = true;

// Serpentine Index Mapping
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

namespace LEDManager {

void init() {
    // Initialize FastLED with GRB color order on LED_PIN
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    // Set a safe global brightness level (26 out of 255, approx 10% brightness)
    // to prevent drawing excessive current from a USB power source
    FastLED.setBrightness(26);
    FastLED.clear();
    FastLED.show();
    
    Serial.println("[LED] LED Manager Initialized. FastLED Ready.");
}

// 1. Diagnostic Heart Pattern (Phase 2 legacy)
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
    float netEnv = env - 200.0f;
    if (netEnv < 0.0f) netEnv = 0.0f;

    float maxRef = 45000.0f;
    float normEnv = netEnv / maxRef;
    if (normEnv > 1.0f) normEnv = 1.0f;
    float soundFactor = sqrt(normEnv);

    float baselineScale = 0.3f + 0.8f * soundFactor;
    float pulseScale = baselineScale * (0.85f + 0.15f * basePulse);

    float cx = (MATRIX_WIDTH - 1) / 2.0f;     
    float cy = (MATRIX_HEIGHT - 1) / 2.0f + 1; 

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            float dx = (x - cx) / (4.5f * pulseScale);
            float dy = (y - cy) / (5.0f * pulseScale);
            
            float a = dx * dx + dy * dy - 1.0f;
            float heartVal = a * a * a - dx * dx * dy * dy * dy;

            uint16_t idx = getLEDIndex(x, y);
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

// 2. Helper to draw a single frequency spectrum column with peak fall-off
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

    if (delta > 8000.0f && env > 4000.0f) { // Transient sound trigger
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

// 8. Idle Simplex Noise Mode
void drawNoise() {
    static uint32_t zDist = 0;
    zDist += 3; // Shift speed of noise space
    
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            // Generate Simplex noise: x-scale, y-scale, z-distance
            uint8_t noiseVal = inoise8(x * 35, y * 35, zDist);
            
            // Map the noise value to a smooth, transitioning hue gradient (e.g. violet to turquoise)
            uint8_t hue = 160 + (noiseVal / 4); // 160 (Blue/Indigo) to ~224 (Pink/Crimson)
            uint8_t sat = 255 - (noiseVal / 8);  // Slight saturation breathing
            uint8_t bri = dim8_raw(noiseVal);    // Use FastLED's smooth dimming
            
            leds[getLEDIndex(x, y)] = CHSV(hue, sat, bri);
        }
    }
}

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
            // Diagonal swirling wave
            uint8_t hue = (uint8_t)(x * 8 + y * 12 + hueOffset);
            leds[getLEDIndex(x, y)] = CHSV(hue, sat, bri);
        }
    }
}

// 10. Fire Portal (Audio-reactive flame simulation rising from the bottom)
void drawFirePortal() {
    static uint8_t heat[MATRIX_WIDTH][MATRIX_HEIGHT] = {{0}};
    float env = AudioProcessor::getVolumeEnvelope();
    
    // 1. Cool down: every cell cools down randomly
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            uint8_t cooldown = random(3, 8);
            if (cooldown >= heat[x][y]) {
                heat[x][y] = 0;
            } else {
                heat[x][y] -= cooldown;
            }
        }
    }

    // 2. Drift up: heat flows upward and diffuses
    for (int y = MATRIX_HEIGHT - 1; y >= 2; y--) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            int xLeft = (x > 0) ? x - 1 : x;
            int xRight = (x < MATRIX_WIDTH - 1) ? x + 1 : x;
            heat[x][y] = (heat[x][y - 1] + heat[xLeft][y - 1] + heat[xRight][y - 1] + heat[x][y - 2]) / 4;
        }
    }
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        heat[x][1] = (heat[x][0] + ((x > 0) ? heat[x - 1][0] : heat[x][0]) + ((x < MATRIX_WIDTH - 1) ? heat[x + 1][0] : heat[x][0])) / 3;
    }

    // 3. Ignite: Feed bottom row (y=0) with heat based on sound level
    int baseHeat = 80 + constrain((int)(env / 15.0f), 0, 175);
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        if (random8() < 128) {
            heat[x][0] = qadd8(heat[x][0], random(baseHeat / 2, baseHeat));
        }
    }

    // 4. Map heat values to fire colors (Black -> Red -> Yellow -> White)
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            uint8_t h = heat[x][y];
            CRGB color;
            if (h < 85) {
                color = CRGB(h * 3, 0, 0);
            } else if (h < 170) {
                color = CRGB(255, (h - 85) * 3, 0);
            } else {
                color = CRGB(255, 255, (h - 170) * 3);
            }
            leds[getLEDIndex(x, y)] = color;
        }
    }
}

// 11. Digital Rain (Neon green matrix rain whose speed scales with audio volume)
void drawDigitalRain() {
    static float rainY[MATRIX_WIDTH] = {0};
    static float rainSpeed[MATRIX_WIDTH] = {0};
    static uint8_t initDone = 0;
    
    if (!initDone) {
        for (int i = 0; i < MATRIX_WIDTH; i++) {
            rainY[i] = random(0, MATRIX_HEIGHT);
            rainSpeed[i] = 0.08f + random(4, 12) / 100.0f;
        }
        initDone = 1;
    }
    
    // Fade display down slowly to create falling trails
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].fadeToBlackBy(45);
    }
    
    float env = AudioProcessor::getVolumeEnvelope();
    // Speed multiplier driven by audio envelope
    float speedMult = 1.0f + (env / 2500.0f) * 1.8f;
    
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        rainY[x] -= rainSpeed[x] * speedMult;
        if (rainY[x] < 0) {
            rainY[x] = MATRIX_HEIGHT - 1;
            rainSpeed[x] = 0.06f + random(3, 10) / 100.0f;
        }
        
        int headY = (int)rainY[x];
        if (headY >= 0 && headY < MATRIX_HEIGHT) {
            // Neon cyan-green palette
            uint8_t hue = 96 + random(0, 32);
            leds[getLEDIndex(x, headY)] = CHSV(hue, 255, 255);
        }
    }
}

// 12. Pulsing Tunnel (Concentric color rings expanding from center, pulsing to the beat)
void drawPulsingTunnel() {
    // Fade screen slowly to create beautiful trails
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].fadeToBlackBy(50);
    }

    static float timeOffset = 0;
    float env = AudioProcessor::getVolumeEnvelope();
    
    // Speed up ring expansion with audio volume
    float speed = 0.4f + (env / 2500.0f) * 2.0f;
    timeOffset += speed;
    
    float cx = (MATRIX_WIDTH - 1) / 2.0f;
    float cy = (MATRIX_HEIGHT - 1) / 2.0f;
    
    static uint8_t baseHue = 0;
    baseHue += 1;
    
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            
            // Wave phase expanding outward
            uint8_t phase = (uint8_t)(dist * 35.0f - timeOffset);
            uint8_t wave = sin8(phase);
            
            if (wave > 200) {
                uint8_t bri = map(wave, 200, 255, 0, 255);
                // Dynamic brightness boost based on volume
                uint8_t volBoost = constrain((int)(env / 20.0f), 0, 150);
                uint8_t finalBri = qadd8(bri, volBoost);
                
                uint8_t hue = baseHue + (uint8_t)(dist * 15);
                leds[getLEDIndex(x, y)] += CHSV(hue, 240, finalBri);
            }
        }
    }
}

void update() {
    // Choose rendering function based on active mode
    switch (currentMode) {
        case MODE_DIAGNOSTIC_HEART:
            drawDiagnosticHeart();
            break;
        case MODE_SPECTRUM_LINEAR:
            drawSpectrumLinear();
            break;
        case MODE_SPECTRUM_SYMMETRIC:
            drawSpectrumSymmetric();
            break;
        case MODE_VU_METER:
            drawVUMeter();
            break;
        case MODE_BASS_PULSE:
            drawBassPulse();
            break;
        case MODE_SOUND_RIPPLES:
            drawSoundRipples();
            break;
        case MODE_NOISE:
            drawNoise();
            break;
        case MODE_RAINBOW_WAVE:
            drawRainbowWave();
            break;
        case MODE_FIRE_PORTAL:
            drawFirePortal();
            break;
        case MODE_DIGITAL_RAIN:
            drawDigitalRain();
            break;
        case MODE_PULSING_TUNNEL:
            drawPulsingTunnel();
            break;
        default:
            FastLED.clear();
            break;
    }
    
    FastLED.show();
}

void setMode(VisualizerMode mode) {
    if (mode < MODE_COUNT) {
        currentMode = mode;
        FastLED.clear();
        Serial.printf("[LED] Mode changed to: %s\n", getModeName(currentMode));
    }
}

void nextMode() {
    uint8_t next = (uint8_t)currentMode + 1;
    if (next >= MODE_COUNT) {
        next = 0;
    }
    setMode((VisualizerMode)next);
}

const char* getModeName(VisualizerMode mode) {
    switch (mode) {
        case MODE_DIAGNOSTIC_HEART:   return "Diagnostic Heart";
        case MODE_SPECTRUM_LINEAR:    return "Linear Spectrum";
        case MODE_SPECTRUM_SYMMETRIC: return "Symmetric Spectrum";
        case MODE_VU_METER:           return "Stereo VU Meter";
        case MODE_BASS_PULSE:         return "Bass Pulse";
        case MODE_SOUND_RIPPLES:      return "Sound Ripples";
        case MODE_NOISE:              return "Ambient Noise";
        case MODE_RAINBOW_WAVE:       return "Rainbow Wave";
        case MODE_FIRE_PORTAL:        return "Fire Portal";
        case MODE_DIGITAL_RAIN:       return "Digital Rain";
        case MODE_PULSING_TUNNEL:     return "Pulsing Tunnel";
        default:                      return "Unknown";
    }
}

VisualizerMode getActiveMode() {
    return currentMode;
}

void setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
}

uint8_t getBrightness() {
    return FastLED.getBrightness();
}

void setAutoCycle(bool enabled) {
    autoCycleEnabled = enabled;
}

bool getAutoCycle() {
    return autoCycleEnabled;
}

} // namespace LEDManager

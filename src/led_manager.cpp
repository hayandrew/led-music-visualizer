#include "led_manager.h"
#include "project_config.h"
#include "audio_processor.h"
#include <FastLED.h>

static CRGB leds[NUM_LEDS];
static VisualizerMode currentMode = MODE_SPECTRUM_LINEAR; // Start with linear spectrum
static bool autoCycleEnabled = false;

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
    
    struct Spark {
        float x;
        float y;
        float vx;
        float vy;
        uint8_t life;
        bool active;
    };
    static Spark sparks[10] = {0};

    float env = AudioProcessor::getVolumeEnvelope();
    
    // 1. Dynamic cooling rate based on volume
    // Quiet -> fast cooling (speeds up decay). Loud -> slow cooling (lets flames rise).
    int coolMin = map(env, 0, 3500, 8, 3);
    coolMin = constrain(coolMin, 3, 10);
    int coolMax = map(env, 0, 3500, 16, 6);
    coolMax = constrain(coolMax, 6, 20);

    for (int x = 0; x < MATRIX_WIDTH; x++) {
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            uint8_t cooldown = random(coolMin, coolMax);
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

    // 3. Dynamic Height Cap based on volume (flames grow/shrink physically)
    int maxHeight = map(env, 0, 3000, 5, MATRIX_HEIGHT);
    maxHeight = constrain(maxHeight, 5, MATRIX_HEIGHT);
    for (int y = maxHeight; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            heat[x][y] = (heat[x][y] * 1) / 2; // Fast decay above height cap
        }
    }

    // 4. Ignite: Feed bottom row (y=0) with heat based on sound level
    int baseHeat = 20 + constrain((int)(env / 12.0f), 0, 235);
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        if (random8() < 120) {
            heat[x][0] = qadd8(heat[x][0], random(baseHeat / 2, baseHeat));
        }
    }

    // 5. Render heat to fire colors
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            uint8_t h = heat[x][y];
            CRGB color;
            if (h < 85) {
                color = CRGB(h * 3, 0, 0); // Red
            } else if (h < 170) {
                color = CRGB(255, (h - 85) * 3, 0); // Yellow
            } else {
                color = CRGB(255, 255, (h - 170) * 3); // White hot
            }
            leds[getLEDIndex(x, y)] = color;
        }
    }

    // 6. Spawn and Render Crackling Sparks/Embers
    // Higher volume increases spark chance
    int spawnChance = 15 + constrain((int)(env / 100.0f), 0, 60);
    if (random8() < spawnChance) {
        for (int i = 0; i < 10; i++) {
            if (!sparks[i].active) {
                sparks[i].active = true;
                sparks[i].x = random(0, MATRIX_WIDTH);
                sparks[i].y = random(0, 3); // Start near bottom
                sparks[i].vx = (random(-5, 6) / 10.0f); // Horizontal drift
                sparks[i].vy = 0.6f + (random(4, 12) / 10.0f); // Drift up speed
                sparks[i].life = random(120, 255);
                break;
            }
        }
    }

    // Update and draw active sparks
    for (int i = 0; i < 10; i++) {
        if (sparks[i].active) {
            sparks[i].x += sparks[i].vx;
            sparks[i].y += sparks[i].vy;
            if (sparks[i].life > 15) {
                sparks[i].life -= 12; // Decay
            } else {
                sparks[i].life = 0;
            }

            int sx = (int)sparks[i].x;
            int sy = (int)sparks[i].y;

            if (sy >= MATRIX_HEIGHT || sx < 0 || sx >= MATRIX_WIDTH || sparks[i].life == 0) {
                sparks[i].active = false;
            } else {
                // Draw ember overlay: orange-white hot
                uint16_t idx = getLEDIndex(sx, sy);
                leds[idx] = CRGB(
                    qadd8(leds[idx].r, sparks[i].life),
                    qadd8(leds[idx].g, (sparks[i].life * 3) / 4),
                    qadd8(leds[idx].b, sparks[i].life / 2)
                );
            }
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
    static float timeOffset = 0;
    float env = AudioProcessor::getVolumeEnvelope();
    
    // Speed up ring expansion with audio volume
    float speed = 1.2f + (env / 2000.0f) * 4.5f;
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
            
            // Higher spatial frequency for multiple concurrent rings
            uint8_t phase = (uint8_t)(dist * 65.0f - timeOffset);
            uint8_t wave = sin8(phase);
            
            // Shape the wave to get distinct, crisp rings with black gaps
            uint8_t bri = 0;
            if (wave > 100) {
                bri = map(wave, 100, 255, 0, 255);
            }
            
            // Entire tunnel pulses in brightness with the beat
            uint8_t volBoost = constrain((int)(env / 20.0f), 0, 130);
            uint8_t finalBri = qadd8(bri, volBoost);
            
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

// 13. Super Mario Run (Mario sprite running in place, speed driven by audio)
// 0: Black/Transparent, 1: Red (Cap/Overalls), 2: Skin (Orange/Face), 3: Olive Green (Shirt/Hair/Shoes)
static const uint8_t mario_frames[4][16][12] = {
    // Frame 0: Standing
    {
        {0,0,0,1,1,1,1,1,0,0,0,0},
        {0,0,1,1,1,1,1,1,1,1,1,0},
        {0,0,3,3,3,2,2,3,2,0,0,0},
        {0,3,2,3,2,2,2,3,2,2,2,0},
        {0,3,2,3,3,2,2,2,3,2,2,2},
        {0,3,3,2,2,2,2,3,3,3,3,0},
        {0,0,0,2,2,2,2,2,2,2,0,0},
        {0,0,1,1,3,1,1,1,0,0,0,0},
        {0,1,1,1,3,1,1,3,1,1,1,0},
        {1,1,1,1,3,3,3,3,1,1,1,1},
        {2,2,1,3,2,3,3,2,3,1,2,2},
        {2,2,2,3,3,3,3,3,3,2,2,2},
        {2,2,3,3,3,3,3,3,3,3,2,2},
        {0,0,3,3,3,0,0,3,3,3,0,0},
        {0,3,3,3,0,0,0,0,3,3,3,0},
        {3,3,3,3,0,0,0,0,3,3,3,3}
    },
    // Frame 1: Run 1
    {
        {0,0,0,0,1,1,1,1,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,1},
        {0,0,0,3,3,3,2,2,3,2,0,0},
        {0,0,3,2,3,2,2,2,3,2,2,2},
        {0,0,3,2,3,3,2,2,2,3,2,2},
        {0,0,3,3,2,2,2,2,3,3,3,3},
        {0,0,0,0,2,2,2,2,2,2,2,0},
        {0,0,0,3,3,1,3,3,3,0,0,0},
        {0,0,3,3,3,1,3,3,1,3,0,0},
        {0,3,3,3,3,1,1,1,1,3,3,0},
        {2,2,3,3,1,2,1,1,2,1,3,2},
        {2,2,2,1,1,1,1,1,1,1,2,2},
        {0,2,1,1,1,1,1,1,1,1,2,0},
        {0,0,1,1,1,0,0,1,1,1,0,0},
        {0,3,3,3,0,0,0,0,3,3,3,0},
        {3,3,3,3,0,0,0,0,3,3,3,3}
    },
    // Frame 2: Run 2
    {
        {0,0,0,1,1,1,1,1,0,0,0,0},
        {0,0,1,1,1,1,1,1,1,1,1,0},
        {0,0,3,3,3,2,2,3,2,0,0,0},
        {0,3,2,3,2,2,2,3,2,2,2,0},
        {0,3,2,3,3,2,2,2,3,2,2,2},
        {0,3,3,2,2,2,2,3,3,3,3,0},
        {0,0,0,2,2,2,2,2,2,2,0,0},
        {0,0,1,1,3,1,1,1,3,0,0,0},
        {0,1,1,1,3,1,1,3,1,1,0,0},
        {1,1,1,1,3,3,3,3,1,1,1,0},
        {2,2,2,3,2,1,1,2,3,1,1,2},
        {0,2,2,1,1,1,1,1,1,1,2,2},
        {0,0,1,1,1,1,1,1,1,1,2,0},
        {0,1,1,1,0,0,0,1,1,1,0,0},
        {0,3,3,0,0,0,0,0,3,3,0,0},
        {3,3,3,0,0,0,0,0,3,3,3,0}
    },
    // Frame 3: Run 3
    {
        {0,0,0,0,1,1,1,1,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,1},
        {0,0,0,3,3,3,2,2,3,2,0,0},
        {0,0,3,2,3,2,2,2,3,2,2,2},
        {0,0,3,2,3,3,2,2,2,3,2,2},
        {0,0,3,3,2,2,2,2,3,3,3,3},
        {0,0,0,0,2,2,2,2,2,2,2,0},
        {0,0,1,1,3,1,1,1,3,0,0,0},
        {0,1,1,1,3,1,1,3,1,1,0,0},
        {1,1,1,1,3,3,3,3,1,1,1,0},
        {2,2,2,3,2,1,1,2,3,1,1,2},
        {0,2,2,1,1,1,1,1,1,1,2,2},
        {0,0,1,1,1,1,1,1,1,1,2,0},
        {0,1,1,1,0,0,0,1,1,1,0,0},
        {0,3,3,0,0,0,0,0,3,3,0,0},
        {3,3,3,0,0,0,0,0,3,3,3,0}
    }
};

void drawMarioRun() {
    // Clear screen completely
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
    }

    float env = AudioProcessor::getVolumeEnvelope();

    // Determine running frame loop based on audio envelope
    static float animFrame = 1.0f;
    int currentFrame = 0; // Standing by default

    if (env > 400.0f) { // Sound threshold to start running
        // Running speed matches volume level
        float speed = 0.08f + (env / 3000.0f) * 0.35f;
        animFrame += speed;
        if (animFrame >= 4.0f) {
            animFrame = 1.0f; // Loop run frames: 1, 2, 3
        }
        currentFrame = (int)animFrame;
    } else {
        animFrame = 1.0f;
        currentFrame = 0; // Stand still
    }

    // Center the 12-wide, 16-high sprite on the 15x17 grid
    int x_offset = (MATRIX_WIDTH - 12) / 2;  // (15 - 12) / 2 = 1
    int y_offset = (MATRIX_HEIGHT - 16) / 2; // (17 - 16) / 2 = 0 (rows 0 to 15, leaving 16 blank)

    for (int row = 0; row < 16; row++) {
        int y_pixel = y_offset + (15 - row);
        if (y_pixel < 0 || y_pixel >= MATRIX_HEIGHT) continue;

        for (int col = 0; col < 12; col++) {
            int x_pixel = x_offset + col;
            if (x_pixel < 0 || x_pixel >= MATRIX_WIDTH) continue;

            uint8_t pixelColorIdx = mario_frames[currentFrame][row][col];
            if (pixelColorIdx == 0) continue; // Transparent

            CRGB color;
            switch (pixelColorIdx) {
                case 1: color = CRGB(192, 28, 12); break;     // Red cap & overalls
                case 2: color = CRGB(240, 156, 28); break;    // Skin orange
                case 3: color = CRGB(80, 92, 12); break;      // Olive green shirt/hair/shoes
                default: color = CRGB::Black; break;
            }
            leds[getLEDIndex(x_pixel, y_pixel)] = color;
        }
    }
}

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
    timeOffset += 0.012f; // Smooth speed of floating blobs

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
    hueOffset += 0.035f;
    uint8_t blobHue = (uint8_t)hueOffset;
    uint8_t bgHue = (uint8_t)(hueOffset + 96); // Contrasting liquid background

    float env = AudioProcessor::getVolumeEnvelope();
    // Brighter/darker bubbles depending on sound volume
    float soundMult = 0.15f + (env / 3500.0f) * 0.85f;
    soundMult = constrain(soundMult, 0.15f, 1.0f);

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

void update() {
    // Choose rendering function based on active mode
    switch (currentMode) {
        case MODE_DIAGNOSTIC_HEART:
            drawDiagnosticHeart();
            break;
        case MODE_SPECTRUM_LINEAR:
            drawSpectrumLinear();
            break;
        // case MODE_SPECTRUM_SYMMETRIC:
        //     drawSpectrumSymmetric();
        //     break;
        // case MODE_VU_METER:
        //     drawVUMeter();
        //     break;
        // case MODE_BASS_PULSE:
        //     drawBassPulse();
        //     break;
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
        case MODE_MARIO_RUN:
            drawMarioRun();
            break;
        case MODE_LAVA_LAMP:
            drawLavaLamp();
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
        // case MODE_SPECTRUM_SYMMETRIC: return "Symmetric Spectrum";
        // case MODE_VU_METER:           return "Stereo VU Meter";
        // case MODE_BASS_PULSE:         return "Bass Pulse";
        case MODE_SOUND_RIPPLES:      return "Sound Ripples";
        case MODE_NOISE:              return "Ambient Noise";
        case MODE_RAINBOW_WAVE:       return "Rainbow Wave";
        case MODE_FIRE_PORTAL:        return "Fire Portal";
        case MODE_DIGITAL_RAIN:       return "Digital Rain";
        case MODE_PULSING_TUNNEL:     return "Pulsing Tunnel";
        case MODE_MARIO_RUN:          return "Super Mario Run";
        case MODE_LAVA_LAMP:          return "Lava Lamp";
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

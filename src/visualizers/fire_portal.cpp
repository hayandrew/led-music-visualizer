#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

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
    
    // 1. Dynamic cooling rate based on volume (Tuned sensitivity: mapped up to 11000)
    int coolMin = map(env, 0, 11000, 10, 4);
    coolMin = constrain(coolMin, 4, 12);
    int coolMax = map(env, 0, 11000, 20, 8);
    coolMax = constrain(coolMax, 8, 24);

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

    // 2. Drift up: heat flows upward only within the same column (Isolated Channels)
    for (int y = MATRIX_HEIGHT - 1; y >= 2; y--) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            heat[x][y] = (heat[x][y - 1] * 3 + heat[x][y - 2]) / 4;
        }
    }
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        heat[x][1] = (heat[x][0] * 2 + heat[x][1]) / 3;
    }

    // 3. Dynamic Height Cap based on volume (with smooth per-column variation)
    int maxHeight = map(env, 0, 11000, 2, MATRIX_HEIGHT);
    maxHeight = constrain(maxHeight, 2, MATRIX_HEIGHT);
    
    static uint16_t timeOffset = 0;
    timeOffset += 15; // Speed of the flame height variation waves
    
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        // Use Simplex Noise to generate smooth height offsets per column
        uint8_t noiseVal = inoise8(x * 65, timeOffset);
        int variation = map(noiseVal, 0, 255, -3, 3); // variation of +/- 3 rows
        int colMaxHeight = maxHeight + variation;
        colMaxHeight = constrain(colMaxHeight, 2, MATRIX_HEIGHT);
        
        for (int y = colMaxHeight; y < MATRIX_HEIGHT; y++) {
            heat[x][y] = heat[x][y] / 2; // Fast decay above this column's height cap
        }
    }

    // 4. Ignite: Feed bottom row based on sound level (Tuned sensitivity: env/55.0f instead of env/80.0f)
    int baseHeat = 10 + constrain((int)(env / 55.0f), 0, 140);
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        if (random8() < 82) { // 32% chance per column instead of 27%
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

    // 6. Spawn and Render Crackling Sparks/Embers (Tuned sensitivity: env/300.0f instead of env/400.0f)
    int spawnChance = 6 + constrain((int)(env / 300.0f), 0, 25);
    if (random8() < spawnChance) {
        for (int i = 0; i < 10; i++) {
            if (!sparks[i].active) {
                sparks[i].active = true;
                sparks[i].x = random(0, MATRIX_WIDTH);
                sparks[i].y = random(0, 3);
                sparks[i].vx = (random(-3, 4) / 10.0f); // Less horizontal drift to stay inside channel
                sparks[i].vy = 0.6f + (random(4, 12) / 10.0f);
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
                sparks[i].life -= 12;
            } else {
                sparks[i].life = 0;
            }

            int sx = (int)sparks[i].x;
            int sy = (int)sparks[i].y;

            if (sy >= MATRIX_HEIGHT || sx < 0 || sx >= MATRIX_WIDTH || sparks[i].life == 0) {
                sparks[i].active = false;
            } else {
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

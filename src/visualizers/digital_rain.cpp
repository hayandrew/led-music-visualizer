#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

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

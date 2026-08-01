#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

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

    // Running speed matches volume level (constantly runs, speeds up or down depending on sound)
    float speed = 0.05f + (env / 2000.0f) * 0.40f;
    speed = constrain(speed, 0.05f, 0.45f);

    static float animFrame = 0.0f;
    animFrame += speed;
    if (animFrame >= 4.0f) {
        animFrame -= 4.0f; // Cycle through all 4 frames: 0, 1, 2, 3
    }
    int currentFrame = (int)animFrame;

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

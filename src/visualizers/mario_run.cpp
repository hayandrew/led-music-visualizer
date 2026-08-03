#include "visualizers.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 13. Super Mario Run (Mario sprite running in place, speed driven by audio)
// 0: Black/Transparent, 1: Red (Cap/Overalls), 2: Skin (Orange/Face), 3: Olive Green (Shirt/Hair/Shoes)
struct SpriteFrame {
    uint8_t width;
    uint8_t height;
    const uint8_t* pixels;
};

// Sprite 0: media__1785615588387.png (14x15)
static const uint8_t mario_sprite_0[15][14] = {
    {0,0,0,0,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,3,3,3,2,2,3,2,2,0,0,0},
    {0,0,3,2,3,2,2,2,3,2,2,2,2,0},
    {0,0,3,2,3,3,2,2,2,3,2,2,2,2},
    {0,0,3,3,2,2,2,2,3,3,3,3,3,0},
    {0,0,0,0,2,2,2,2,2,2,2,2,0,0},
    {0,0,0,3,3,3,3,1,3,3,2,2,0,0},
    {0,0,2,3,3,3,3,3,3,2,2,2,2,0},
    {0,2,2,1,3,3,3,3,3,2,2,2,0,0},
    {0,3,3,1,1,1,1,1,1,1,1,0,0,0},
    {0,3,1,1,1,1,1,1,1,1,1,0,0,0},
    {3,3,1,1,1,1,1,1,1,1,0,0,0,0},
    {3,3,0,0,0,3,3,3,3,0,0,0,0,0},
    {0,0,0,0,0,3,3,3,3,3,0,0,0,0}
};

// Sprite 1: media__1785615612278.png (12x17)
static const uint8_t mario_sprite_1[17][12] = {
    {0,0,1,1,1,1,1,1,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,0},
    {0,3,3,3,2,2,3,2,2,0,0,0},
    {3,2,3,2,2,2,3,2,2,2,2,0},
    {3,2,3,3,2,2,2,3,2,2,2,2},
    {3,3,2,2,2,2,3,3,3,3,3,0},
    {0,0,2,2,2,2,2,2,2,2,0,0},
    {0,3,3,1,3,3,3,3,0,0,0,0},
    {3,3,3,3,1,1,3,3,3,0,0,0},
    {3,3,3,1,1,2,1,1,2,2,0,0},
    {3,3,3,3,1,1,1,1,1,1,0,0},
    {1,3,3,2,2,2,1,1,1,1,0,0},
    {0,1,3,2,2,1,1,1,1,0,0,0},
    {0,0,1,1,1,3,3,3,3,0,0,0},
    {0,0,3,3,3,3,3,3,3,3,0,0},
    {0,0,3,3,3,3,3,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};

// Sprite 2: media__1785615626935.png (16x16)
static const uint8_t mario_sprite_2[16][16] = {
    {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,3,3,3,2,2,3,2,2,0,0,0,0},
    {0,0,0,3,2,3,2,2,2,3,2,2,2,2,0,0},
    {0,0,0,3,2,3,3,2,2,2,3,2,2,2,2,0},
    {0,0,0,3,3,2,2,2,2,3,3,3,3,3,0,0},
    {0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0},
    {0,0,3,3,3,3,1,1,3,3,3,0,0,0,0,0},
    {2,2,3,3,3,3,1,1,1,3,3,3,2,2,2,2},
    {2,2,2,2,3,3,1,2,1,1,1,3,3,2,2,2},
    {2,2,2,0,1,1,1,1,1,1,1,1,0,3,3,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,3,3,3,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,3,3,3,0},
    {0,3,3,1,1,1,1,0,0,1,1,1,3,3,3,0},
    {0,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,3,3,3,3,0,0,0,0,0,0,0,0,0,0}
};

static const SpriteFrame mario_frames[3] = {
    {14, 15, (const uint8_t*)mario_sprite_0},
    {12, 17, (const uint8_t*)mario_sprite_1},
    {16, 16, (const uint8_t*)mario_sprite_2}
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
    if (animFrame >= 3.0f) {
        animFrame -= 3.0f; // Cycle through all 3 frames: 0, 1, 2
    }
    int currentFrame = (int)animFrame;

    const SpriteFrame& frame = mario_frames[currentFrame];

    // Center the sprite on the 15x17 grid (handling potentially negative offsets for larger sprites)
    int x_offset = MATRIX_WIDTH - (int)frame.width;
    if (x_offset < 0) {
        x_offset = (x_offset - 1) / 2;
    } else {
        x_offset = x_offset / 2;
    }

    int y_offset = MATRIX_HEIGHT - (int)frame.height;
    if (y_offset < 0) {
        y_offset = (y_offset - 1) / 2;
    } else {
        y_offset = y_offset / 2;
    }

    for (int row = 0; row < frame.height; row++) {
        int y_pixel = y_offset + (frame.height - 1 - row);
        if (y_pixel < 0 || y_pixel >= MATRIX_HEIGHT) continue;

        for (int col = 0; col < frame.width; col++) {
            int x_pixel = x_offset + col;
            if (x_pixel < 0 || x_pixel >= MATRIX_WIDTH) continue;

            uint8_t pixelColorIdx = frame.pixels[row * frame.width + col];
            if (pixelColorIdx == 0) continue; // Transparent

            CRGB color;
            switch (pixelColorIdx) {
                case 1: color = CRGB(192, 28, 12); break;     // Classic NES Red (Cap & Overalls)
                case 2: color = CRGB(240, 156, 28); break;   // Classic NES Skin (Peach)
                case 3: color = CRGB(80, 92, 12); break;    // Classic NES Brown (Shirt/Hair/Shoes)
                default: color = CRGB::Black; break;
            }
            leds[getLEDIndex(x_pixel, y_pixel)] = color;
        }
    }
}

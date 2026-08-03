#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>

// 3x5 font map (low 5 bits of each byte define rows top-to-bottom: bit 4 is top, bit 0 is bottom)
static const uint8_t font3x5[][3] = {
    { 0x00, 0x00, 0x00 }, // 0: Space
    { 0x1F, 0x14, 0x1F }, // 1: A
    { 0x1F, 0x15, 0x0A }, // 2: B
    { 0x0E, 0x11, 0x11 }, // 3: C
    { 0x1F, 0x11, 0x0E }, // 4: D
    { 0x1F, 0x15, 0x15 }, // 5: E
    { 0x11, 0x1F, 0x11 }, // 6: I
    { 0x1F, 0x04, 0x1B }, // 7: K
    { 0x1F, 0x01, 0x01 }, // 8: L
    { 0x1F, 0x08, 0x1F }, // 9: N
    { 0x1F, 0x14, 0x1B }, // 10: R
    { 0x09, 0x15, 0x12 }, // 11: S
    { 0x1F, 0x01, 0x1F }  // 12: U
};

// Character index sequence for "LIKE AND SUBSCRIBE"
static const uint8_t textSequence[] = {
    8, 6, 7, 5,       // L, I, K, E
    0,                // Space
    1, 9, 4,          // A, N, D
    0,                // Space
    11, 12, 2, 11, 3, 10, 6, 2, 5  // S, U, B, S, C, R, I, B, E
};

void drawSubscribe() {
    // Leave color trails for a smooth scrolling effect
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].fadeToBlackBy(70);
    }

    // Determine scrolling speed based on current audio volume
    float env = AudioProcessor::getVolumeEnvelope();
    float speed = 0.5f + (env / 4000.0f);
    if (speed > 2.0f) speed = 2.0f; // Cap max scroll speed

    static float scrollX = MATRIX_WIDTH;
    scrollX -= speed;

    const int textLen = sizeof(textSequence);
    const int charSpacing = 4; // 3 columns for font + 1 column of padding
    const int scale = 2;       // Double the scale for legibility on the 15x17 grid

    int totalWidth = textLen * charSpacing * scale;

    // Reset scroll position when text has fully scrolled off screen
    if (scrollX < -totalWidth) {
        scrollX = MATRIX_WIDTH;
    }

    static uint8_t baseHue = 0;
    baseHue++; // Dynamic color shift over time

    float currentX = scrollX;
    
    // Draw each character in the sequence
    for (int i = 0; i < textLen; i++) {
        uint8_t charIdx = textSequence[i];
        
        // Skip drawing if character is out of view (optimization)
        if (currentX + charSpacing * scale >= 0 && currentX < MATRIX_WIDTH) {
            
            // Loop through 3 columns of the character font
            for (int col = 0; col < 3; col++) {
                uint8_t colData = font3x5[charIdx][col];
                float drawX = currentX + col * scale;
                
                // Loop through 5 rows of the character font
                for (int row = 0; row < 5; row++) {
                    // Check if the pixel bit (4 - row) is set
                    if (colData & (1 << (4 - row))) {
                        float drawY = 3.0f + (4 - row) * scale; // Center vertically on 17-row matrix (corrected for vertical orientation)
                        
                        // Draw scaled pixel block (2x2)
                        for (int dx = 0; dx < scale; dx++) {
                            for (int dy = 0; dy < scale; dy++) {
                                int px = (int)(drawX + dx);
                                int py = (int)(drawY + dy);
                                
                                if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
                                    uint16_t idx = getLEDIndex(px, py);
                                    // Map gradient hue across the screen columns
                                    leds[idx] = CHSV(baseHue + px * 5, 240, 255);
                                }
                            }
                        }
                    }
                }
            }
        }
        currentX += charSpacing * scale;
    }
}

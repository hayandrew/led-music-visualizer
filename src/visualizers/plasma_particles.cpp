#include "visualizers.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "project_config.h"
#include <FastLED.h>



// 16. Audio Particles (Fireflies rising from bottom of matrix, reacting to audio)
struct Particle {
    float x, y;
    float vx, vy;
    uint8_t hue;
    float life; // 0.0 to 1.0
    bool active;
};

const int MAX_PARTICLES = 25;
static Particle particles[MAX_PARTICLES];

void drawAudioParticles() {
    // Fade the existing frame slightly to leave trailing trails
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].fadeToBlackBy(80); // Quick fade
    }

    float* bands = AudioProcessor::getFrequencyBands();
    float bass = max(bands[0], bands[1]);
    float mids = (bands[2] + bands[3] + bands[4]) / 3.0f;
    float treble = max(bands[5], bands[6]);

    // Spawn new particles based on bass beat
    // Higher bass = higher probability to spawn, and spawns more particles
    static unsigned long lastSpawn = 0;
    unsigned long now = millis();
    if (bass > 0.4f && (now - lastSpawn > 80)) {
        lastSpawn = now;
        // Find an inactive particle
        int spawns = (bass > 0.75f) ? 2 : 1;
        for (int s = 0; s < spawns; s++) {
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (!particles[i].active) {
                    particles[i].active = true;
                    particles[i].x = random(0, MATRIX_WIDTH * 10) / 10.0f;
                    particles[i].y = 0.0f; // Start at the bottom
                    // velocity: rise speed driven partly by mids
                    particles[i].vy = 0.12f + mids * 0.25f + (random(0, 100) / 1000.0f);
                    // horizontal drift driven by treble
                    particles[i].vx = (random(-100, 100) / 300.0f) * (1.0f + treble * 1.5f);
                    // color matches the beat (low frequencies are warm, higher frequencies make them purple/blue)
                    particles[i].hue = (uint8_t)(random8(10, 45) + treble * 120);
                    particles[i].life = 1.0f;
                    break;
                }
            }
        }
    }

    // Update and draw active particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            // Apply velocities
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;

            // Add slight random wandering
            particles[i].vx += (random(-50, 50) / 1000.0f);
            // Constrain x within screen boundaries with bounce
            if (particles[i].x < 0) {
                particles[i].x = 0;
                particles[i].vx *= -1;
            } else if (particles[i].x >= MATRIX_WIDTH) {
                particles[i].x = MATRIX_WIDTH - 1;
                particles[i].vx *= -1;
            }

            // Reduce life
            particles[i].life -= 0.015f + (1.0f - particles[i].vy) * 0.005f;

            // Deactivate if out of bounds or dead
            if (particles[i].y >= MATRIX_HEIGHT || particles[i].life <= 0.0f) {
                particles[i].active = false;
                continue;
            }

            // Draw particle onto the led buffer
            int px = (int)round(particles[i].x);
            int py = (int)round(particles[i].y);
            if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
                uint16_t idx = getLEDIndex(px, py);
                uint8_t bri = (uint8_t)(particles[i].life * 255.0f);
                leds[idx] = CHSV(particles[i].hue, 240, bri);
            }
        }
    }
}

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display_manager.h"
#include "controls_manager.h"
#include "project_config.h"
#include "led_manager.h"
#include "audio_processor.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Share reset pin with ESP32
#define SCREEN_ADDRESS 0x3C // Standard I2C address for SSD1306

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace DisplayManager {
    // Scrolling diagnostics buffer
    static float history[128] = {0.0f};
    static int historyIndex = 0;

    // Helper to get shortened mode names that fit on the 128px screen width
    const char* getShortModeName(VisualizerMode mode) {
        switch (mode) {
            case MODE_DIAGNOSTIC_HEART:   return "Heartbeat";
            case MODE_SPECTRUM_LINEAR:     return "Linear Spec";
            case MODE_SPECTRUM_SYMMETRIC:   return "Sym Spec";
            case MODE_VU_METER:           return "VU Meter";
            case MODE_BASS_PULSE:         return "Bass Pulse";
            case MODE_SOUND_RIPPLES:       return "Ripples";
            case MODE_NOISE:               return "Noise";
            default:                       return "Visualizer";
        }
    }

    void init() {
        Serial.println("[Display] Initializing Custom I2C for SSD1306...");
        Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            Serial.println("[Display] SSD1306 allocation failed. Check wiring!");
            return;
        }

        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.display();
        Serial.println("[Display] OLED screen ready.");
    }

    void update() {
        // Redraw at ~30 FPS
        static uint32_t lastDraw = 0;
        if (millis() - lastDraw < 33) return;
        lastDraw = millis();

        // 1. Update the scrolling volume history buffer
        history[historyIndex] = AudioProcessor::getVolumeEnvelope();
        historyIndex = (historyIndex + 1) % 128;

        // 2. Clear buffers and start drawing
        display.clearDisplay();

        // Header Title
        display.setTextSize(1);
        display.setCursor(4, 0);
        display.print("=== LED VISUALIZER ===");
        display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

        // Fetch active settings and controls state
        int cursor = ControlsManager::getMenuCursor();
        bool editing = ControlsManager::isEditing();

        // Render Menu Items
        for (int i = 0; i < 4; i++) {
            int y = 12 + i * 9;
            display.setCursor(2, y);

            // Display item labels
            if (i == 0) display.print("Mode: ");
            else if (i == 1) display.print("Bright: ");
            else if (i == 2) display.print("Gain: ");
            else if (i == 3) display.print("Auto: ");

            // Cursor and Brackets Formatting
            if (cursor == i) {
                if (editing) {
                    display.print(">"); // Bracket indicating active parameter edit
                } else {
                    display.print("["); // Bracket indicating active menu navigation selection
                }
            } else {
                display.print(" ");
            }

            // Render current values
            if (i == 0) {
                display.print(getShortModeName(LEDManager::getActiveMode()));
            } else if (i == 1) {
                int pct = (LEDManager::getBrightness() * 100) / 255;
                display.print(pct);
                display.print("%");
            } else if (i == 2) {
                display.print(AudioProcessor::getGain(), 1);
                display.print("x");
            } else if (i == 3) {
                display.print(LEDManager::getAutoCycle() ? "ON" : "OFF");
            }

            // Close brackets
            if (cursor == i) {
                if (editing) {
                    display.print("<");
                } else {
                    display.print("]");
                }
            }
        }

        // Draw Divider Line for diagnostics window
        display.drawFastHLine(0, 49, 128, SSD1306_WHITE);

        // Render Scrolling Diagnostics Waveform
        float maxVal = 100.0f; // Minimal floor to prevent tiny noise from auto-maximizing
        for (int j = 0; j < 128; j++) {
            if (history[j] > maxVal) maxVal = history[j];
        }

        for (int x = 0; x < 128; x++) {
            int idx = (historyIndex + x) % 128;
            float val = history[idx];
            
            // Map value into a 12px height container (bottom y-rows 51 to 63)
            int barHeight = (int)((val / maxVal) * 11.0f);
            if (barHeight > 11) barHeight = 11;
            
            // Draw a vertical line from the bottom (63) upward
            display.drawFastVLine(x, 63 - barHeight, barHeight + 1, SSD1306_WHITE);
        }

        // Render the buffer to the physical screen
        display.display();
    }
}

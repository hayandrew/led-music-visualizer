#include "controls_manager.h"
#include "project_config.h"
#include "led_manager.h"
#include "audio_processor.h"

namespace ControlsManager {
    ControlState currentState = STATE_NAV;
    int menuCursor = 0;
    const int NUM_MENU_ITEMS = 4;

    // Interrupt state variables
    volatile int rawEncoderDelta = 0;
    volatile uint8_t prevState = 0;
    
    void IRAM_ATTR handleEncoderISR() {
        // Read current state of CLK and DT
        uint8_t currState = (digitalRead(ENCODER_CLK_PIN) << 1) | digitalRead(ENCODER_DT_PIN);
        
        // Form 4-bit index: (prev_A, prev_B, curr_A, curr_B)
        uint8_t index = (prevState << 2) | currState;
        prevState = currState;
        
        switch (index) {
            // Clockwise transitions
            case 0b1101: // 11 -> 01
            case 0b0100: // 01 -> 00
            case 0b0010: // 00 -> 10
            case 0b1011: // 10 -> 11
                rawEncoderDelta++;
                break;
                
            // Counter-clockwise transitions
            case 0b1110: // 11 -> 10
            case 0b1000: // 10 -> 00
            case 0b0001: // 00 -> 01
            case 0b0111: // 01 -> 11
                rawEncoderDelta--;
                break;
        }
    }

    void init() {
        // Configure encoder pins with internal pullups
        pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
        pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
        pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

        // Initialize starting state
        prevState = (digitalRead(ENCODER_CLK_PIN) << 1) | digitalRead(ENCODER_DT_PIN);

        // Attach CHANGE interrupts to BOTH CLK and DT for full quadrature tracking
        attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), handleEncoderISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), handleEncoderISR, CHANGE);
    }

    void update() {
        // 1. Read and clear encoder raw delta
        int rawDelta = 0;
        noInterrupts();
        rawDelta = rawEncoderDelta;
        rawEncoderDelta = 0;
        interrupts();

        // 2. Accumulate ticks and divide by 4 to get detent steps
        static int accum = 0;
        int delta = 0;
        if (rawDelta != 0) {
            accum += rawDelta;
            if (abs(accum) >= 4) {
                delta = accum / 4;
                accum = accum % 4;
            }
        }

        // 2. Read SW switch press with debouncing
        static bool lastSwState = HIGH;
        static uint32_t lastSwDebounceTime = 0;
        bool currentSwState = digitalRead(ENCODER_SW_PIN);
        bool swClicked = false;

        if (currentSwState != lastSwState) {
            if (millis() - lastSwDebounceTime > 50) {
                if (currentSwState == LOW) {
                    swClicked = true;
                    Serial.println("[Controls] Encoder switch clicked.");
                }
                lastSwState = currentSwState;
                lastSwDebounceTime = millis();
            }
        }

        // 3. Process inputs based on state machine
        if (currentState == STATE_NAV) {
            // Handle menu navigation
            if (delta != 0) {
                menuCursor += delta;
                // Clamp menu cursor between 0 and NUM_MENU_ITEMS - 1
                if (menuCursor < 0) menuCursor = NUM_MENU_ITEMS - 1;
                if (menuCursor >= NUM_MENU_ITEMS) menuCursor = 0;
                
                Serial.printf("[Controls] Menu cursor: %d\n", menuCursor);
            }

            if (swClicked) {
                currentState = STATE_EDIT;
                Serial.printf("[Controls] Entered EDIT mode for item %d\n", menuCursor);
            }
        } else if (currentState == STATE_EDIT) {
            // Handle editing values
            if (delta != 0) {
                switch (menuCursor) {
                    case 0: { // Mode Selection
                        int currentModeInt = (int)LEDManager::getActiveMode();
                        currentModeInt += delta;
                        if (currentModeInt < 0) currentModeInt = (int)MODE_COUNT - 1;
                        if (currentModeInt >= (int)MODE_COUNT) currentModeInt = 0;
                        
                        LEDManager::setMode((VisualizerMode)currentModeInt);
                        Serial.printf("[Controls] Mode changed to: %s\n", LEDManager::getModeName((VisualizerMode)currentModeInt));
                        break;
                    }
                    case 1: { // Brightness (Step by 25, 10 to 255)
                        int b = LEDManager::getBrightness();
                        b += delta * 25;
                        if (b < 10) b = 10;       // Minimum brightness
                        if (b > 255) b = 255;     // Maximum brightness
                        
                        LEDManager::setBrightness((uint8_t)b);
                        Serial.printf("[Controls] Brightness changed to: %d\n", b);
                        break;
                    }
                    case 2: { // Gain (Step by 0.2, 0.2 to 5.0)
                        float g = AudioProcessor::getGain();
                        g += delta * 0.2f;
                        if (g < 0.2f) g = 0.2f;
                        if (g > 5.0f) g = 5.0f;
                        
                        AudioProcessor::setGain(g);
                        Serial.printf("[Controls] Gain changed to: %.1f\n", g);
                        break;
                    }
                    case 3: { // Auto-Cycle (Toggle)
                        bool ac = LEDManager::getAutoCycle();
                        ac = !ac; // Any turn toggles it
                        LEDManager::setAutoCycle(ac);
                        Serial.printf("[Controls] Auto-Cycle changed to: %s\n", ac ? "ON" : "OFF");
                        break;
                    }
                }
            }

            if (swClicked) {
                currentState = STATE_NAV;
                Serial.println("[Controls] Returned to NAV mode.");
            }
        }
    }

    ControlState getState() {
        return currentState;
    }

    int getMenuCursor() {
        return menuCursor;
    }

    bool isEditing() {
        return (currentState == STATE_EDIT);
    }
}

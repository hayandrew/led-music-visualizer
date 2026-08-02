#include "controls_manager.h"
#include "project_config.h"
#include "led_manager.h"
#include "audio_processor.h"
#include <cmath>

namespace ControlsManager {
    ControlState currentState = STATE_NAV;
    int menuCursor = 0;
    const int NUM_MENU_ITEMS = 4;

    // State machine states
    #define R_START 0x0
    #define R_CW_FINAL 0x1
    #define R_CW_BEGIN 0x2
    #define R_CW_NEXT 0x3
    #define R_CCW_BEGIN 0x4
    #define R_CCW_FINAL 0x5
    #define R_CCW_NEXT 0x6

    #define DIR_CW 0x10
    #define DIR_CCW 0x20

    // Transition table for full-step encoder
    static const uint8_t ttable[7][4] = {
        // R_START
        {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
        // R_CW_FINAL
        {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
        // R_CW_BEGIN
        {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
        // R_CW_NEXT
        {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
        // R_CCW_BEGIN
        {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
        // R_CCW_FINAL
        {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
        // R_CCW_NEXT
        {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
    };

    // Interrupt state variables
    volatile int rawEncoderDelta = 0;
    volatile uint8_t encoderState = R_START;
    
    void IRAM_ATTR handleEncoderISR() {
        // Read current state of CLK and DT
        uint8_t pinState = (digitalRead(ENCODER_CLK_PIN) << 1) | digitalRead(ENCODER_DT_PIN);
        
        // Lookup next state
        encoderState = ttable[encoderState & 0x0f][pinState];
        
        // Check if we completed a rotation
        uint8_t result = encoderState & 0x30;
        if (result == DIR_CW) {
            rawEncoderDelta++;
        } else if (result == DIR_CCW) {
            rawEncoderDelta--;
        }
    }

    void init() {
        // Configure encoder pins with internal pullups
        pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
        pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
        pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

        // Initialize starting state
        encoderState = R_START;

        // Attach CHANGE interrupts to BOTH CLK and DT for full quadrature tracking
        attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), handleEncoderISR, CHANGE);
        attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), handleEncoderISR, CHANGE);
    }

    void update() {
        // 1. Read and clear encoder raw delta (each tick is exactly one detent click)
        int delta = 0;
        noInterrupts();
        delta = rawEncoderDelta;
        rawEncoderDelta = 0;
        interrupts();

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
                    case 1: { // Brightness (Step by 5%, 0% to 100%)
                        int currentPct = (int)round((LEDManager::getBrightness() * 100.0) / 255.0);
                        int nextPct = currentPct + delta * 5;
                        if (nextPct < 0) nextPct = 0;
                        if (nextPct > 100) nextPct = 100;
                        
                        int b = (nextPct * 255) / 100;
                        LEDManager::setBrightness((uint8_t)b);
                        Serial.printf("[Controls] Brightness changed to: %d%% (%d/255)\n", nextPct, b);
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

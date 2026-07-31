# Implementation Plan: ESP32-C3 LED Visualizer (Phased Approach)

This document outlines the phased development plan for an audio-reactive LED visualizer using an ESP32-C3 microcontroller.

## Hardware Configuration
- **LED Matrix**: 15 columns by 17 rows (255 WS2812B LEDs total) wired in a serpentine (zigzag) pattern.
- **Audio Sensor**: INMP441 digital I2S microphone.
- **Manual Input & Output UI**:
  - **Rotary Encoder**: Controls navigation and parameter changes.
  - **I2C 1" OLED Display** (e.g. 128x64 SSD1306): Shows menus, active visualizer mode, brightness, speed, and Wi-Fi state.

---

## Phased Development Roadmap

### Phase 1: LED Diagnostics & OTA Setup (Complete)
*Goal: Bring up the ESP32-C3, test the power distribution and integrity of the serpentine matrix, and enable Over-The-Air (OTA) firmware flashing.*
1. **PlatformIO Project Initialization**: Set up the project target for the ESP32-C3.
2. **Wi-Fi & ArduinoOTA Initialization**: Start a Wi-Fi Access Point (SSID: `ESP32C3-Visualizer`) and launch ArduinoOTA.
3. **LED Diagnostic Sequence**:
   - Diagonal 2D Rainbow Wave pattern.
   - Math-modeled pulsating Red Heartbeat with dim breathing background.
   - Primary colors (R, G, B, W) at safe brightness levels.
   - Serpentine pixel trace mapping.
4. **Current Pinout**:
   - **LED Data Pin**: **GPIO 2**

### Phase 2: I2S Audio Acquisition (INMP441 Microphone) (Next Phase)
*Goal: Capture clean digital audio from the INMP441 microphone using the ESP32-C3's I2S peripheral and DMA.*
1. Configure I2S interface parameters:
   - **SCK (Serial Clock)**: **GPIO 8**
   - **WS (Word Select)**: **GPIO 3**
   - **SD (Serial Data)**: **GPIO 4**
   - **L/R (Left/Right)**: Connect to GND (Channels Left)
2. Implement double-buffered DMA sampling to continuously read 32-bit audio samples (filtered to 24-bit).
3. Compute audio signal statistics (peak-to-peak amplitude, moving average envelope) to verify the microphone is capturing audio successfully.

### Phase 3: FFT Analysis & Core Animations
*Goal: Compute frequency bands using FFT and build the basic visualizers.*
1. Install `arduinoFFT` library.
2. Compute Fast Fourier Transform (FFT) on audio buffers.
3. Group frequencies into 7 visualizer bands (Sub-Bass to Treble) and calculate dynamic gain adjustments (auto-gain) to scale the response.
4. Implement matrix mapping code (serpentine translation: maps `(x,y)` coordinate to 1D index).
5. Build core animations:
   - *VU Amplitude Meter*: Volume drives height of columns.
   - *Frequency Spectrum*: Columns represent bands, height shows amplitude.
   - *Bass Pulse*: Bass triggers color changes or ring expansions.
   - *Procedural Noise/Fire*: Visuals run when no audio is present.

### Phase 4: Web UI Dashboard
*Goal: Create a beautiful, responsive dark-mode Web UI hosted on the ESP32-C3 to control settings.*
1. Set up `ESPAsyncWebServer`.
2. Build single-page web application featuring:
   - Dynamic dark dashboard design with glassmorphism.
   - Brightness, speed, and audio sensitivity sliders.
   - Interactive palette and animation selector.
3. Establish HTTP API endpoints to sync configurations between Web UI and ESP32-C3 settings.

### Phase 5: System Integration & Optimization
*Goal: Save settings to non-volatile memory, optimize performance, and run final system testing.*
1. Integrate Preferences library to persist settings (active mode, brightness, speed, sensitivity) across reboots.
2. Optimize execution loop (run animations on a dedicated timer at 60 FPS, separate from Wi-Fi/web tasks).
3. Verify overall stability and thermal levels.

### Phase 6: Physical Controls & Menu UI (Rotary Encoder & I2C Display)
*Goal: Set up the physical control interface using a rotary encoder and a 1-inch I2C OLED screen.*
1. Install `ESP32Encoder` library, `Adafruit SSD1306`, and `Adafruit GFX Library`.
2. Configure **GPIO 0 (SDA)** and **GPIO 1 (SCL)** for I2C communication.
3. Configure GPIO interrupts for encoder rotation (`CLK` on GPIO 5, `DT` on GPIO 6) and button presses (`SW` on GPIO 7).
4. Design a clean, responsive display dashboard and menu framework:
   - **Home Screen**: Displays the current visualizer mode, Wi-Fi status, brightness, and audio sensitivity.
   - **Menu Screen**: Allows the user to scroll through visualizer modes (Rainbow, Fire, VU Meter, Spectrum, etc.) using the encoder knob and select a mode by clicking the encoder button.
   - **Setting Mode**: Clicking the button can toggle between adjusting "Mode Selection", "Brightness", or "Speed/Gain" when rotating the encoder.

---

## Complete Hardware Pin Mapping (Proposed)

| Component | Signal | ESP32-C3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **WS2812B Matrix** | Data | **GPIO 2** | Needs FastLED, supports RMT |
| **INMP441 Mic** | SCK | **GPIO 8** | I2S Serial Clock |
| | WS | **GPIO 3** | I2S Word Select |
| | SD | **GPIO 4** | I2S Serial Data |
| **SSD1306 Display** | SDA | **GPIO 0** | I2C Data (Phase 6) |
| | SCL | **GPIO 1** | I2C Clock (Phase 6) |
| **Rotary Encoder** | CLK | **GPIO 5** | Encoder Channel A (Phase 6) |
| | DT | **GPIO 6** | Encoder Channel B (Phase 6) |
| | SW | **GPIO 7** | Push Button (Phase 6) |

---

## Phase 2 Implementation Details

For Phase 2, we will create the following files in the workspace:

### [MODIFY] [platformio.ini](file:///Users/andyhay/workspace/led-visualizer/platformio.ini)
- No new libraries needed yet, standard I2S drivers are part of the Espressif ESP32 Core.

### [MODIFY] [config.h](file:///Users/andyhay/workspace/led-visualizer/include/config.h)
- Central config additions:
  - I2S Microphone pin mappings (`I2S_SCK = 8`, `I2S_WS = 3`, `I2S_SD = 4`).
  - Audio configuration parameters (Sample Rate = 16000Hz, FFT size / buffer sizes).

### [NEW] [audio_processor.h](file:///Users/andyhay/workspace/led-visualizer/include/audio_processor.h) & [audio_processor.cpp](file:///Users/andyhay/workspace/led-visualizer/src/audio_processor.cpp)
- Set up ESP32-C3 I2S peripheral using standard Arduino `driver/i2s.h` ESP-IDF configuration.
- Read microphone samples asynchronously using FreeRTOS DMA tasks.
- Provide real-time volume envelope metrics and raw audio amplitude to the main loop.

### [MODIFY] [main.cpp](file:///Users/andyhay/workspace/led-visualizer/src/main.cpp)
- Initialize the I2S microphone at startup.
- In the diagnostic patterns or main loop, print live audio readings to the Serial monitor to verify sound capture is operational.

---

## Verification Plan (Phase 2)

### Automated Tests
- Build code locally to check for compile errors:
  ```bash
  ~/Library/Python/3.9/bin/pio run
  ```

### Manual Verification
1. Flash firmware (wireless OTA upload or USB):
   ```bash
   pio run --target upload
   ```
2. Open Serial Monitor:
   ```bash
   pio device monitor
   ```
3. Clap or play music near the mic and verify that the printed peak amplitude and moving volume values rise and fall dynamically.

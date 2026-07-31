# ESP32-C3 LED Music Visualizer

An audio-reactive LED music visualizer built on the ESP32-C3 microcontroller, using a WS2812B LED matrix and an INMP441 I2S digital microphone.

---

## 🛠 Hardware Configuration

The project utilizes the following hardware configurations defined in `include/project_config.h`:

### LED Matrix
* **Geometry:** 15 Columns × 17 Rows (Serpentine layout)
* **Total LEDs:** 255
* **LED Pin:** `GPIO 2`

### I2S Microphone (INMP441)
* **I2S SCK Pin (Serial Clock):** `GPIO 8`
* **I2S WS Pin (Word Select):** `GPIO 3`
* **I2S SD Pin (Serial Data):** `GPIO 4`
* **Sampling Rate:** 16,000 Hz

### Wi-Fi Access Point Settings
By default, the microcontroller hosts its own Wi-Fi network for OTA updates:
* **SSID:** `ESP32C3-Visualizer`
* **Password:** `12345678`
* **OTA Port:** `3232`

---

## 🗺 Development Roadmap

This project is divided into several iterative phases:

### **Phase 1: LED Diagnostics & OTA Setup** (✅ Complete)
* Set up basic PlatformIO project.
* Programmed the column-major serpentine matrix coordinate mapping.
* Verified LED connection with a pulsating algebraic heartbeat diagnostics pattern.
* Configured wireless OTA update callbacks to flash the device over Wi-Fi.

### **Phase 2: Audio Capture & Envelope Tracking** (✅ Complete)
* Configured ESP32-C3 I2S interface to sample raw microphone data in a background FreeRTOS task.
* Implemented a DC blocking IIR filter to remove offset bias from raw samples.
* Implemented volume envelope and peak amplitude tracking.
* Connected the volume envelope to scale the diagnostics LED heartbeat animation in real-time.

### **Phase 3: FFT Frequency Analysis** (⏳ Remaining)
* Integrate an FFT library (e.g., `arduinoFFT` or Espressif's `esp-dsp`).
* Run frequency analysis on the audio sample buffer.
* Sort frequencies into groups (Bass, Mids, Treble) for precise frequency-based animations.

### **Phase 4: Audio-Reactive Visualization Modes** (⏳ Remaining)
* Implement spectrum analyzer bars mapped to the 15x17 grid.
* Add a volume-responsive color VU meter.
* Develop frequency-reactive custom effects (e.g., bass pulsing, color organ, lava/fire ripples).

### **Phase 5: Web Configuration Portal** (⏳ Remaining)
* Serve a responsive web dashboard from the ESP32 AP.
* Add sliders for global brightness, microphone gain, and noise gate threshold.
* Implement buttons to cycle through visualizer modes and select customized color palettes.
* Persist user configurations in non-volatile flash memory (SPIFFS/Preferences).

---

## 🚀 Getting Started

### Prerequisites
* [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE](https://platformio.org/) extension installed.
* ESP32-C3 development board.

### Build and Upload
1. Clone the repository and open the folder in VS Code / PlatformIO.
2. Build the project:
   ```bash
   pio run
   ```
3. Upload the firmware over USB:
   ```bash
   pio run --target upload
   ```
4. Subsequent updates can be flashed wirelessly (OTA) by connecting to the `ESP32C3-Visualizer` Wi-Fi AP and uploading:
   ```bash
   pio run --target upload --upload-port <ESP32-AP-IP>
   ```

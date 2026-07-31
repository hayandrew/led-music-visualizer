# Walkthrough: Phase 2 (I2S Audio Acquisition)

We have successfully completed Phase 2 of the ESP32-C3 LED Visualizer project. The microphone has been wired, and we have confirmed it is capturing audio dynamically. 

The diagnostic visualizer has been locked to the **Pulsating Heartbeat** pattern, which now scales its size in real-time based on the volume level captured by the digital microphone.

---

## 🛠️ Changes Implemented

1. **[platformio.ini](file:///Users/andyhay/workspace/led-visualizer/platformio.ini)**:
   - Locked the monitor port to `/dev/cu.usbmodem2101` to prevent auto-detecting Bluetooth peripherals.
   - Added build flags `-D ARDUINO_USB_MODE=1` and `-D ARDUINO_USB_CDC_ON_BOOT=1` to direct the standard C++ `Serial` console logging to the native USB port.

2. **[include/project_config.h](file:///Users/andyhay/workspace/led-visualizer/include/project_config.h)**:
   - Configured digital I2S pin mappings for the **INMP441** microphone:
     - **SCK**: GPIO 8
     - **WS**: GPIO 3
     - **SD**: GPIO 4
   - Defined sampling rate constants (16 kHz, 256 samples per DMA block).

3. **[include/audio_processor.h](file:///Users/andyhay/workspace/led-visualizer/include/audio_processor.h)** & **[src/audio_processor.cpp](file:///Users/andyhay/workspace/led-visualizer/src/audio_processor.cpp)**:
   - Configured the ESP32-C3 I2S controller in Master Receiver mode.
   - Spawned a high-priority background FreeRTOS task (`AudioCaptureTask`) to stream raw 24-bit audio data over DMA without locking the animation framerate.
   - Implemented an IIR DC Block filter to isolate audio signals from static voltage bias.
   - Computed peak-to-peak amplitude and a slow-decay volume envelope to drive animations.

4. **[src/led_diagnostics.cpp](file:///Users/andyhay/workspace/led-visualizer/src/led_diagnostics.cpp)**:
   - Retained the **Heartbeat** pattern as the sole visualization.
   - Linked the heart's baseline scale to `AudioProcessor::getVolumeEnvelope()`. 
   - Silence produces a small, gentle heart in the center of the grid, while talking, clapping, or music swells the heart to fill the canvas.
   - Synced the heartbeat's red brightness (crimson pulse) with the audio intensity.

5. **[src/main.cpp](file:///Users/andyhay/workspace/led-visualizer/src/main.cpp)**:
   - Instantiated and started `AudioProcessor::init()`.
   - Added 100ms periodic status logging showing peak values.

---

## 🚀 How to Flash the Sound-Reactive Heart

Since the compilation builds successfully, you can flash the update:

### Option A: Flash via USB
```bash
pio run --target upload --upload-port /dev/cu.usbmodem2101
```

### Option B: Flash via OTA (Wi-Fi)
1. Join the Wi-Fi AP: `ESP32C3-Visualizer` (Password: `12345678`).
2. Run:
   ```bash
   pio run --target upload --upload-port 192.168.4.1
   ```

### Verification
Once flashed, open your serial monitor (`pio device monitor`) or look directly at your matrix:
- Speak or clap next to the microphone. 
- You should see the heart grow larger and pulse brighter in sync with the sound level!

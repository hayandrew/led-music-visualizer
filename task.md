# Tasks: Phase 3 (FFT Analysis & Core Animations)

- `[ ]` Install `kosme/arduinoFFT` dependency in `platformio.ini`
- `[ ]` Update `include/audio_processor.h` and `src/audio_processor.cpp` to run FFT calculation on incoming buffers
- `[ ]` Create `include/led_manager.h` and `src/led_manager.cpp` to structure animation engines
- `[ ]` Move the serpentine coordinate translation to `led_manager.cpp` and implement animations:
  - *Spectrum Waterfall / Columns visualizer* (7 frequency bands mapped to matrix columns)
  - *Advanced VU Meter* (Dynamic volume-reactive level heights)
  - *Bass Pulse / Ring expansion* (Low frequency triggers pulsing waves)
  - *Sound Ripples* (Sound peaks spawn wave expansions)
  - *Procedural Noise* (Runs when no audio is present)
- `[ ]` Integrate visualizer engine into `main.cpp` and compile locally
- `[ ]` Flash to the ESP32-C3 and test the audio visualizers with music

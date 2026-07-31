#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <Arduino.h>

namespace AudioProcessor {
    // Initialize the I2S interface and start the background sampling task
    void init();

    // Get the smoothed volume envelope (useful for VU meters)
    float getVolumeEnvelope();

    // Get the raw peak-to-peak amplitude from the most recent buffer
    float getPeakAmplitude();

    // Get a pointer to the buffer of DC-removed float samples (size = I2S_BUFFER_SIZE)
    float* getAudioBuffer();

    // Check if a new buffer of samples has been read and is ready for FFT processing
    bool isNewBufferReady();

    // Reset the new buffer flag after reading
    void clearNewBufferFlag();
}

#endif // AUDIO_PROCESSOR_H

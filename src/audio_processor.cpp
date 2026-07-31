#include "audio_processor.h"
#include "project_config.h"
#include "driver/i2s.h"

static float audioBuffer[I2S_BUFFER_SIZE];
static volatile float volumeEnvelope = 0.0f;
static volatile float peakAmplitude = 0.0f;
static volatile bool newBufferReady = false;

static TaskHandle_t audioTaskHandle = NULL;

namespace AudioProcessor {

void audioCaptureTask(void* parameter) {
    int32_t i2s_raw_samples[I2S_BUFFER_SIZE];
    size_t bytes_read = 0;
    
    // DC Offset removal variables
    float last_x = 0;
    float last_y = 0;

    Serial.println("[Audio] Capture task started.");

    while (true) {
        // Read from I2S DMA. This blocks until the buffer is full (about 16ms for 256 samples at 16kHz)
        esp_err_t result = i2s_read(I2S_NUM_0, 
                                    i2s_raw_samples, 
                                    sizeof(i2s_raw_samples), 
                                    &bytes_read, 
                                    portMAX_DELAY);
        
        if (result == ESP_OK && bytes_read > 0) {
            uint16_t samples_count = bytes_read / sizeof(int32_t);
            float currentMax = -999999.0f;
            float currentMin = 999999.0f;
            
            for (uint16_t i = 0; i < samples_count; i++) {
                // Convert 32-bit slot sample (24-bit MSB-aligned data)
                // Shift right by 14 to convert to a reasonable float range
                float raw_val = (float)(i2s_raw_samples[i] >> 14);
                
                // Apply a standard IIR DC Block filter: y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
                float filtered = raw_val - last_x + 0.995f * last_y;
                last_x = raw_val;
                last_y = filtered;
                
                audioBuffer[i] = filtered;
                
                // Track min and max for peak-to-peak amplitude
                if (filtered > currentMax) currentMax = filtered;
                if (filtered < currentMin) currentMin = filtered;
            }
            
            // Calculate peak-to-peak amplitude
            float p2p = currentMax - currentMin;
            if (p2p < 0.0f) p2p = 0.0f;
            
            peakAmplitude = p2p;
            
            // Apply slow-decay envelope tracking
            // If the new peak is higher, rise instantly; if lower, decay slowly
            if (p2p > volumeEnvelope) {
                volumeEnvelope = volumeEnvelope * 0.4f + p2p * 0.6f;
            } else {
                volumeEnvelope = volumeEnvelope * 0.92f + p2p * 0.08f;
            }
            
            newBufferReady = true;
        }
        
        // Yield briefly
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void init() {
    Serial.println("[Audio] Initializing I2S Interface...");

    // 1. Configure I2S Driver Settings
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // Master Receiver
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,        // INMP441 uses 32-bit slot width
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,         // Left channel (L/R pin grounded)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,            // Interrupt level 1
        .dma_buf_count = 4,                                  // 4 DMA buffers
        .dma_buf_len = I2S_BUFFER_SIZE,                      // 256 samples each
        .use_apll = false,                                   // Do not use APLL clock (ESP32-C3 standard clock is fine)
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // 2. Configure I2S Hardware Pins
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,                   // Receiving only
        .data_in_num = I2S_SD_PIN
    };

    // 3. Install I2S Driver
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[Audio] ERROR: Failed to install I2S driver: %d\n", err);
        return;
    }

    // 4. Set Hardware Pins
    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[Audio] ERROR: Failed to set I2S pins: %d\n", err);
        return;
    }

    // 5. Create Background Processing Task
    // Pinned to core 0 (single core on C3 anyway) with high priority (5)
    xTaskCreatePinnedToCore(
        audioCaptureTask,
        "AudioCaptureTask",
        4096,
        NULL,
        5,
        &audioTaskHandle,
        0
    );

    Serial.println("[Audio] I2S Interface Initialized Successfully.");
}

float getVolumeEnvelope() {
    return volumeEnvelope;
}

float getPeakAmplitude() {
    return peakAmplitude;
}

float* getAudioBuffer() {
    return audioBuffer;
}

bool isNewBufferReady() {
    return newBufferReady;
}

void clearNewBufferFlag() {
    newBufferReady = false;
}

} // namespace AudioProcessor

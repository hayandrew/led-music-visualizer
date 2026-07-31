#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "project_config.h"
#include "led_manager.h"
#include "audio_processor.h"
#include "web_server_manager.h"

void setup() {
  // Initialize Serial logging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-C3 LED Visualizer Starting ===");

  // Initialize Wi-Fi in Access Point (AP) mode
  Serial.printf("[WiFi] Starting Access Point: %s...\n", AP_SSID);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("[WiFi] AP IP address: ");
  Serial.println(myIP);

  // Set up OTA port (default is 3232, but standard is fine)
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("esp32c3-visualizer");

  // Configure ArduinoOTA event callbacks
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("[OTA] Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] End of update. Rebooting...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] OTA Services Ready.");

  // Initialize I2S Audio Processor
  AudioProcessor::init();

  // Initialize LEDs via Manager
  LEDManager::init();

  // Initialize Web Server Manager
  WebServerManager::init();

  Serial.println("=== Setup Complete. Entering loop ===\n");
}

void loop() {
  // Handle OTA update check
  ArduinoOTA.handle();

  // 1. Run FFT calculation if background I2S buffer is filled
  if (AudioProcessor::isNewBufferReady()) {
    AudioProcessor::runFFT();
    AudioProcessor::clearNewBufferFlag();
  }

  // Update Web Server and push real-time status updates
  WebServerManager::update();
  WebServerManager::broadcastStatus();

  // 2. Auto-cycle visualizer modes every 15 seconds (if enabled)
  static unsigned long lastModeSwitch = millis();
  static VisualizerMode lastActiveMode = LEDManager::getActiveMode();
  VisualizerMode currentMode = LEDManager::getActiveMode();

  if (currentMode != lastActiveMode) {
    lastActiveMode = currentMode;
    lastModeSwitch = millis();
  }

  if (LEDManager::getAutoCycle() && (millis() - lastModeSwitch >= 15000)) {
    lastModeSwitch = millis();
    LEDManager::nextMode();
  }

  // 3. Handle Serial commands to switch modes manually
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'n' || c == ' ') {
      LEDManager::nextMode();
      lastModeSwitch = millis();
    } else if (c >= '0' && c <= '6') {
      LEDManager::setMode((VisualizerMode)(c - '0'));
      lastModeSwitch = millis();
    }
  }

  // 4. Update the visualizer animation frame
  LEDManager::update();

  // Print volume statistics to Serial Monitor every 500ms
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    Serial.printf("[Audio] Peak: %.2f | Env: %.2f | Active Mode: %s\n", 
                  AudioProcessor::getPeakAmplitude(), 
                  AudioProcessor::getVolumeEnvelope(),
                  LEDManager::getModeName(LEDManager::getActiveMode()));
  }

  // Yield to keep the Wi-Fi/IP stack healthy
  delay(10);
}

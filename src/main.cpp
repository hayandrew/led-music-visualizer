#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "project_config.h"
#include "led_diagnostics.h"
#include "audio_processor.h"

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

  // Initialize LEDs
  LEDDiagnostics::init();

  Serial.println("=== Setup Complete. Entering loop ===\n");
}

void loop() {
  // Handle OTA update check
  ArduinoOTA.handle();

  // Run visualizer diagnostic cycles
  LEDDiagnostics::update();

  // Print volume statistics to Serial Monitor every 100ms
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();
    Serial.printf("[Audio] Peak: %.2f | Envelope: %.2f\n", 
                  AudioProcessor::getPeakAmplitude(), 
                  AudioProcessor::getVolumeEnvelope());
  }

  // Yield to keep the Wi-Fi/IP stack healthy
  delay(10);
}

#include "web_server_manager.h"
#include "web_ui.h"
#include "led_manager.h"
#include "audio_processor.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static unsigned long lastBroadcast = 0;

namespace WebServerManager {

// Forward declaration of WebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void init() {
    Serial.println("[Web] Initializing Web Server...");

    // Register WebSocket event handler
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Serve HTML from PROGMEM
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });

    // Start HTTP Server
    server.begin();
    Serial.println("[Web] Web Server and WebSocket Server started on port 80.");
}

void update() {
    // Periodically clean up dead WebSocket connections to free RAM
    ws.cleanupClients();
}

void broadcastStatus() {
    // Throttled broadcast at ~20 FPS (every 50ms) to conserve CPU
    unsigned long now = millis();
    if (now - lastBroadcast >= 50) {
        lastBroadcast = now;

        // Only broadcast if there is at least one active WebSocket client
        if (ws.count() > 0) {
            JsonDocument doc;
            doc["type"] = "stats";
            doc["envelope"] = AudioProcessor::getVolumeEnvelope();
            doc["peak"] = AudioProcessor::getPeakAmplitude();

            JsonArray bands = doc["bands"].to<JsonArray>();
            float* freqBands = AudioProcessor::getFrequencyBands();
            for (int i = 0; i < 7; i++) {
                bands.add(freqBands[i]);
            }

            String output;
            serializeJson(doc, output);
            ws.textAll(output);
        }
    }
}

// WebSocket Event Handler implementation
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[Web] WebSocket Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        
        // Push the active configuration to the newly connected client
        JsonDocument doc;
        doc["type"] = "config";
        doc["brightness"] = LEDManager::getBrightness();
        doc["gain"] = AudioProcessor::getGain();
        doc["autoCycle"] = LEDManager::getAutoCycle();
        doc["mode"] = (uint8_t)LEDManager::getActiveMode();

        String output;
        serializeJson(doc, output);
        client->text(output);
        
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[Web] WebSocket Client #%u disconnected\n", client->id());
        
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            // Null-terminate the text data safely
            data[len] = 0;
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (char*)data);
            if (error) {
                Serial.printf("[Web] JSON deserialization failed: %s\n", error.c_str());
                return;
            }

            bool changed = false;

            if (doc.containsKey("brightness")) {
                uint8_t b = doc["brightness"].as<uint8_t>();
                LEDManager::setBrightness(b);
                changed = true;
                Serial.printf("[Web] Set Brightness to %d\n", b);
            }

            if (doc.containsKey("gain")) {
                float g = doc["gain"].as<float>();
                AudioProcessor::setGain(g);
                changed = true;
                Serial.printf("[Web] Set Gain to %.1f\n", g);
            }

            if (doc.containsKey("mode")) {
                uint8_t m = doc["mode"].as<uint8_t>();
                LEDManager::setMode((VisualizerMode)m);
                changed = true;
                Serial.printf("[Web] Set Mode to %d (%s)\n", m, LEDManager::getModeName((VisualizerMode)m));
            }

            if (doc.containsKey("autoCycle")) {
                bool ac = doc["autoCycle"].as<bool>();
                LEDManager::setAutoCycle(ac);
                changed = true;
                Serial.printf("[Web] Set Auto-Cycle to %s\n", ac ? "ON" : "OFF");
            }

            // If a parameter changed, broadcast the updated configuration to ALL clients
            if (changed) {
                JsonDocument reply;
                reply["type"] = "config";
                reply["brightness"] = LEDManager::getBrightness();
                reply["gain"] = AudioProcessor::getGain();
                reply["autoCycle"] = LEDManager::getAutoCycle();
                reply["mode"] = (uint8_t)LEDManager::getActiveMode();

                String output;
                serializeJson(reply, output);
                ws.textAll(output);
            }
        }
    }
}

} // namespace WebServerManager

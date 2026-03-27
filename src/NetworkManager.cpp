#include "NetworkManager.h"
#include "AudioManager.h"
#include "secrets.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

void NetworkManager::begin(AppController* controller, AudioManager* audio, const char* ssid, const char* pass) {
    appController = controller;
    _audioManager = audio;
    
    Serial.printf("NetworkManager: Connecting to WiFi SSID: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // Initialize WebSocket
        Serial.printf("Connecting to WebSocket at %s:%d...\n", WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT);
        webSocket.begin(WEBSOCKET_SERVER_IP, WEBSOCKET_SERVER_PORT, "/");
        
        webSocket.onEvent([this](WStype_t type, uint8_t * payload, size_t length) {
            this->handleWebSocketEvent(type, payload, length);
        });
        
        webSocket.setReconnectInterval(5000);

    } else {
        Serial.println("\nWiFi connection failed (will keep trying in background).");
    }
}

void NetworkManager::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        _is_connected = false;
        return;
    }
    webSocket.loop();
}

void NetworkManager::handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WS] Disconnected!");
            _is_connected = false;
            break;
        case WStype_CONNECTED:
            Serial.printf("[WS] Connected to url: %s\n", payload);
            _is_connected = true;
            break;
        case WStype_TEXT: {
            Serial.printf("[WS] Received Text: %s\n", payload);
            
            // Phase 3 Upgrade: Parse JSON for audio session control
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                const char* event = doc["event"];
                if (event && strcmp(event, "audio_start") == 0) {
                    int sid = doc["session_id"];
                    if (sid == _current_session_id) {
                        Serial.printf("[WS] Session %d authorized for playback.\n", sid);
                        _active_playback_session_id = sid;
                    } else {
                        Serial.printf("[WS] Ignoring audio_start for stale session %d (Current: %d)\n", sid, _current_session_id);
                    }
                }
            }
            break;
        }
        case WStype_BIN:
            // Only play if the server has authorized the current session!
            if (_audioManager && _active_playback_session_id == _current_session_id) {
                _audioManager->playRaw(payload, length);
            } else {
                // Log dropped packets occasionally so we don't flood the serial but know it's happening
                static unsigned long last_drop_log = 0;
                if (millis() - last_drop_log > 1000) {
                    Serial.printf("[WS] Dropping binary data. Active Session: %d, Current Session: %d\n", _active_playback_session_id, _current_session_id);
                    last_drop_log = millis();
                }
            }
            break;
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

int NetworkManager::startNewSession() {
    _current_session_id++;
    _active_playback_session_id = -1; // Reset authorization
    return _current_session_id;
}

int NetworkManager::getCurrentSessionId() {
    return _current_session_id;
}

bool NetworkManager::isConnected() {
    return _is_connected;
}

void NetworkManager::sendAudio(uint8_t * payload, size_t length) {
    if (_is_connected) {
        webSocket.sendBIN(payload, length);
    }
}

void NetworkManager::sendEvent(const char* eventName) {
    if (_is_connected) {
        webSocket.sendTXT(eventName);
    }
}
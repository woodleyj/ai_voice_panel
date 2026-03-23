#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>

// Forward declaration so we can pass the controller pointer
class AppController;
class AudioManager;

class NetworkManager {
private:
    WebSocketsClient webSocket;
    AppController* appController;
    AudioManager* _audioManager;
    bool _is_connected = false;
    
    int _current_session_id = 0;
    int _active_playback_session_id = -1;

public:
    void begin(AppController* controller, AudioManager* audio, const char* ssid, const char* pass);
    void loop();
    void sendEvent(const char* eventName);
    void sendAudio(uint8_t * payload, size_t length);
    void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    bool isConnected();
    
    int startNewSession();
    int getCurrentSessionId();
};
#include <Arduino.h>
#include "AppController.h"
#include "DisplayManager.h"
#include "AudioManager.h"
#include "NetworkManager.h"
#include "secrets.h"

AppController appController;
DisplayManager displayManager;
AudioManager audioManager;
NetworkManager networkManager;

void setup() {
    Serial.begin(115200);
    delay(2000); // Allow serial monitor to connect
    Serial.println("--- Starting Waveshare Voice Panel ---");

    appController.begin();
    displayManager.begin(); // Initialized first so I2C expander powers up the audio codecs
    audioManager.begin();
    
    networkManager.begin(&appController, &audioManager, WIFI_SSID, WIFI_PASS); 
    
    Serial.println("Setup Complete. Entering Push-To-Talk Loop.");
}

void loop() {
    appController.loop();
    networkManager.loop();
    
    // Update Connection Status on screen
    static bool last_conn = false;
    bool current_conn = networkManager.isConnected();
    if (current_conn != last_conn) {
        displayManager.updateStatus(current_conn ? "Connected" : "Disconnected");
        last_conn = current_conn;
    }
    
    // Read raw touch state and coordinates
    int16_t tx, ty;
    bool raw_touched = displayManager.isTouched(tx, ty);
    
    // Debounce logic for PTT
    static bool is_logically_touched = false;
    static unsigned long last_touched_time = 0;
    unsigned long current_time = millis();

    if (raw_touched) {
        last_touched_time = current_time;
        
        // If it's a new touch, check if it's in the slider area
        if (!is_logically_touched) {
            if (displayManager.isSliderArea(tx, ty)) {
                // Slider logic (non-blocking)
                int slider_x_start = 40;
                int slider_width = 400;
                int val = (tx - slider_x_start) * 230 / slider_width;
                if (val < 0) val = 0;
                if (val > 230) val = 230;
                displayManager.setBrightness(val);
                displayManager.drawUI();
            } else {
                // It's a PTT touch
                is_logically_touched = true;
            }
        } else {
            // Already in a logical touch state (PTT)
            // But if the finger moves into the slider area while holding, 
            // we'll keep it as PTT to prevent interruption.
        }
    } else {
        if (current_time - last_touched_time > 150) {
            is_logically_touched = false;
        }
    }
    
    // State: Interruption check
    if (is_logically_touched && audioManager.isPlaying()) {
        Serial.println("Interruption detected!");
        audioManager.stopPlayback();
        if (networkManager.isConnected()) {
            char event[64];
            snprintf(event, sizeof(event), "{\"event\": \"interruption\", \"session_id\": %d}", networkManager.getCurrentSessionId());
            networkManager.sendEvent(event);
        }
    }

    // State: Recording / Streaming
    if (is_logically_touched) {
        if (!audioManager.isRecording()) {
            displayManager.setStateColor(0xF800); // Red
            
            // Start a fresh session
            int sid = networkManager.startNewSession();
            Serial.printf(">>> Starting Session: %d\n", sid);
            
            if (networkManager.isConnected()) {
                char event[64];
                snprintf(event, sizeof(event), "{\"event\": \"start\", \"session_id\": %d}", sid);
                networkManager.sendEvent(event);
            }
            
            audioManager.startRecording();
            
            // Small 100ms gap to let the server/network prepare before we flood it with binary
            delay(100);
        }
        
        // Read chunk and stream immediately
        uint8_t chunk[512];
        size_t bytes = audioManager.readChunk(chunk, 512);
        if (bytes > 0 && networkManager.isConnected()) {
            networkManager.sendAudio(chunk, bytes);
        }
    } 
    else if (audioManager.isRecording()) {
        int sid = networkManager.getCurrentSessionId();
        Serial.printf("<<< Stopping Session: %d\n", sid);
        audioManager.stopRecording();
        if (networkManager.isConnected()) {
            char event[64];
            snprintf(event, sizeof(event), "{\"event\": \"stop\", \"session_id\": %d}", sid);
            networkManager.sendEvent(event);
        }
    }
    
    // UI Feedback State Machine
    if (audioManager.isRecording()) {
        displayManager.setStateColor(0xF800); // Red
    } else if (audioManager.isPlaying()) {
        displayManager.setStateColor(0xFD20); // Orange
    } else {
        displayManager.setStateColor(0x07E0); // Green
    }
    
    audioManager.loop();
}
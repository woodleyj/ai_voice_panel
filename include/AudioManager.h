#pragma once
#include <stdint.h>
#include <stddef.h>

class AudioManager {
public:
    bool begin();
    void loop();

    void startRecording();
    void stopRecording();
    
    // Server Playback
    void playRaw(uint8_t* data, size_t len);
    void stopPlayback();

    bool isRecording();
    bool isPlaying();
    
    size_t readChunk(uint8_t* buffer, size_t len);

private:
    bool _is_recording = false;
    unsigned long _last_server_packet_time = 0;
};
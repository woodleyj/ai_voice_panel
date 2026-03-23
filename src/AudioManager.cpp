#include "AudioManager.h"
#include "config.h"
#include "AudioTools.h"
#include <Wire.h>
#include <Driver.h>

#if __has_include(<Driver.h>)
AudioDriverES7210Class codec_in;
#define HAS_MIC_DRIVER 1
#else
#define HAS_MIC_DRIVER 0
#endif

AudioDriverES8311Class codec_out;

I2SStream i2s_stream;

bool AudioManager::begin() {
    Serial.println("Initializing Audio Manager (Live Stream Mode)...");

    // I2C Expander was already initialized by DisplayManager!
    audio_driver::DriverPins pins;
    pins.addI2C(audio_driver::PinFunction::CODEC, CUSTOM_I2C_SCL, CUSTOM_I2C_SDA);
    audio_driver::CodecConfig cfg;

    Serial.println("Waking up ES8311 Speaker Codec...");
    codec_out.begin(cfg, pins); 
    codec_out.setVolume(50);

#if HAS_MIC_DRIVER
    Serial.println("Waking up ES7210 Mic Codec...");
    codec_in.begin(cfg, pins);
#endif

    auto cfg_i2s = i2s_stream.defaultConfig(RXTX_MODE);
    cfg_i2s.sample_rate = 16000;
    cfg_i2s.channels = 1;
    cfg_i2s.bits_per_sample = 16;
    cfg_i2s.pin_bck = CUSTOM_I2S_BCLK;
    cfg_i2s.pin_ws = CUSTOM_I2S_WS;
    cfg_i2s.pin_data = CUSTOM_I2S_DOUT; 
    cfg_i2s.pin_data_rx = CUSTOM_I2S_DIN; 
    cfg_i2s.pin_mck = CUSTOM_I2S_MCLK; 
    cfg_i2s.port_no = 0; 
    i2s_stream.begin(cfg_i2s);

    Serial.println("Audio streams configured. Ready.");
    return true;
}

void AudioManager::startRecording() {
    if (_is_recording) return;
    _is_recording = true;
    _last_server_packet_time = 0; // Stop any ongoing server playback UI
    
    // Hard Flush: Drain I2S RX buffer thoroughly
    uint8_t flush_buf[1024];
    for(int i=0; i<10; i++) {
        i2s_stream.readBytes(flush_buf, sizeof(flush_buf));
    }
    
    Serial.println("Streaming Started (Clean Buffer)...");
}

void AudioManager::stopRecording() {
    if (!_is_recording) return;
    _is_recording = false;
    Serial.println("Streaming Stopped.");
}

size_t AudioManager::readChunk(uint8_t* buffer, size_t len) {
    if (!_is_recording) return 0;
    return i2s_stream.readBytes(buffer, len);
}

void AudioManager::playRaw(uint8_t* data, size_t len) {
    if (_is_recording) return; // Don't play while recording (PTT)
    
    // Refresh activity timer for UI
    _last_server_packet_time = millis();
    
    // Ensure PA is definitely ON
    static unsigned long last_pa_check = 0;
    if (millis() - last_pa_check > 5000) {
        Wire.beginTransmission(0x20);
        Wire.write(0x01); 
        Wire.endTransmission();
        Wire.requestFrom(0x20, 1);
        uint8_t out_reg = Wire.read();
        if (!(out_reg & (1 << 3))) {
            out_reg |= (1 << 3);
            Wire.beginTransmission(0x20);
            Wire.write(0x01);
            Wire.write(out_reg);
            Wire.endTransmission();
        }
        last_pa_check = millis();
    }

    i2s_stream.write(data, len);
}

void AudioManager::stopPlayback() {
    _last_server_packet_time = 0;
    // We don't need to do much else as i2s_stream is shared
    Serial.println("Playback Interrupted.");
}

bool AudioManager::isRecording() { return _is_recording; }

bool AudioManager::isPlaying() { 
    return (millis() - _last_server_packet_time < 500); 
}

void AudioManager::loop() {
    // Nothing to do in loop for Live Stream mode,
    // main.cpp handles the data flow.
}
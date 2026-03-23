#pragma once

#include <stdint.h>

class DisplayManager {
public:
    void begin();
    void setScreenColor(uint16_t color);
    
    // Touch handling
    bool isTouched(int16_t &x, int16_t &y);
    bool isSliderArea(int16_t x, int16_t y);
    
    // Brightness
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness();
    
    void updateStatus(const char* status);
    void drawUI();

private:
    uint16_t _current_color = 0;
    uint8_t _brightness = 128;
    char _status[32] = "Disconnected";
    void _drawSlider();
    void _drawStatus();
};
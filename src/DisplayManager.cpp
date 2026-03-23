#include "DisplayManager.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "TouchDrvGT911.hpp"

// Expander for ST7701 Initialization
Arduino_XCA9554SWSPI *expander = nullptr;
Arduino_ESP32RGBPanel *rgbpanel = nullptr;
Arduino_RGB_Display *gfx = nullptr;

TouchDrvGT911 touch;

void DisplayManager::begin() {
    Serial.println("DisplayManager: Initializing I2C for LCD & Audio...");
    
    Wire.begin(CUSTOM_I2C_SDA, CUSTOM_I2C_SCL);
    delay(100);

    // Initialize Expander SPI bus logic
    expander = new Arduino_XCA9554SWSPI(
        7 /* PWD */, 0 /* CS */, 2 /* SCK */, 1 /* MOSI */, &Wire, 0x20
    );

    // Turn on Audio Power & LCD Reset/Backlight manually via the expander
    expander->pinMode(3, OUTPUT); // Audio PA
    expander->digitalWrite(3, HIGH);

    expander->pinMode(6, OUTPUT); // LCD Backlight Enable
    expander->digitalWrite(6, LOW); // Active Low
    delay(200);

    expander->pinMode(5, OUTPUT); // LCD Reset
    expander->digitalWrite(5, LOW);
    delay(200);
    expander->digitalWrite(5, HIGH);
    delay(200);

    // Initialize PWM for Brightness on GPIO 4 using old API for stability
    ledcSetup(0, 5000, 8); 
    ledcAttachPin(CUSTOM_LCD_BLK, 0);
    setBrightness(230);

    // Initialize Display
    Serial.println("DisplayManager: Initializing Arduino_GFX display...");
    rgbpanel = new Arduino_ESP32RGBPanel(
        17 /* DE */, 3 /* VSYNC */, 46 /* HSYNC */, 9 /* PCLK */,
        10 /* B0 */, 11 /* B1 */, 12 /* B2 */, 13 /* B3 */, 14 /* B4 */,
        21 /* G0 */, 8 /* G1 */, 18 /* G2 */, 45 /* G3 */, 38 /* G4 */, 39 /* G5 */,
        40 /* R0 */, 41 /* R1 */, 42 /* R2 */, 2 /* R3 */, 1 /* R4 */,
        1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
        1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */
    );

    gfx = new Arduino_RGB_Display(
        480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation (180 deg) */, true /* auto_flush */,
        expander, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations)
    );

    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    } else {
        Serial.println("gfx->begin() success!");
    }

    // Initialize Touch
    touch.setPins(-1, -1);
    if (touch.begin(Wire, GT911_SLAVE_ADDRESS_H, CUSTOM_I2C_SDA, CUSTOM_I2C_SCL)) {
        Serial.println("Touch GT911 initialized!");
        touch.setMaxTouchPoint(1);
    } else {
        Serial.println("Touch GT911 init failed!");
    }

    // Initial UI render
    gfx->fillScreen(BLACK);
    setStateColor(GREEN); 
}

void DisplayManager::setStateColor(uint16_t color) {
    if (_current_color != color && gfx) {
        _current_color = color;
        drawUI(); // Redraw all UI elements on top of new color
    }
}

void DisplayManager::setBrightness(uint8_t brightness) {
    // // Floor of 15 to prevent PWM flicker at low levels
    // if (brightness > 230) brightness = 230;
    _brightness = brightness;
    ledcWrite(0, _brightness);
}

uint8_t DisplayManager::getBrightness() {
    return _brightness;
}

void DisplayManager::drawUI() {
    _drawButton();
    _drawSlider();
    _drawStatus();
}

void DisplayManager::_drawButton() {
    if (!gfx) return;
    
    // Draw a round button in the center
    int center_x = 240;
    int center_y = 240;
    int radius = 100;

    // Draw button
    gfx->fillCircle(center_x, center_y, radius, _current_color);
    
    // Draw "Alfred" text inside the button
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    
    // Approximate centering of the word "Alfred"
    // Font size 3 roughly means each char is 18px wide and 24px tall
    gfx->setCursor(240 - 54, 240 - 10);
    gfx->print("Alfred");
}

void DisplayManager::updateStatus(const char* status) {
    strncpy(_status, status, sizeof(_status) - 1);
    _status[sizeof(_status) - 1] = '\0';
    _drawStatus();
}

void DisplayManager::_drawStatus() {
    if (!gfx) return;
    
    // Draw high-contrast black status bar
    gfx->fillRect(0, 0, 480, 50, BLACK);
    
    bool connected = (strcmp(_status, "Connected") == 0);
    
    if (connected) {
        // Draw Green Checkmark
        gfx->drawLine(435, 25, 445, 35, GREEN);
        gfx->drawLine(445, 35, 460, 15, GREEN);
        gfx->setTextColor(GREEN);
    } else {
        // Draw Red X
        gfx->drawLine(435, 15, 455, 35, RED);
        gfx->drawLine(455, 15, 435, 35, RED);
        gfx->setTextColor(RED);
    }
    
    gfx->setTextSize(2);
    // 12 pixels width per character at text size 2
    int textWidth = strlen(_status) * 12;
    gfx->setCursor(420 - textWidth, 16);
    gfx->printf("%s", _status);
}

void DisplayManager::_drawSlider() {
    if (!gfx) return;

    int slider_y = 440; // Bottom of the screen in Rotation 2
    int x_start = 40;  // Right side visually in Rot 2
    int x_end = 440;    // Left side visually in Rot 2
    int width = x_end - x_start;

    // Clear the slider strip area (y=400 to 480)
    gfx->fillRect(0, 400, 480, 80, BLACK);

    // Draw track
    gfx->fillRect(x_start, slider_y - 2, width, 4, WHITE);

    // Draw labels
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(x_start, slider_y - 30); 
    gfx->print("Brightness");

    // Draw knob
    // In Rotation 2, X increases to the LEFT.
    // So 0% brightness (far left) is X=440, 100% brightness (far right) is X=40.
    int knob_x = x_end - (_brightness * width / 230);
    gfx->fillCircle(knob_x, slider_y, 15, WHITE);
}

bool DisplayManager::isTouched(int16_t &x, int16_t &y) {
    int16_t tx[1], ty[1];
    if (touch.getPoint(tx, ty, 1) > 0) {
        // Match Rotation 2 (180 degrees)
        // Flip coordinates so that slider at bottom-left is 0,0 and bottom-right is 480,0
        x = tx[0]; 
        y = 480 - ty[0]; // Flip Y so slider area is at y > 380
        return true;
    }
    return false;
}

bool DisplayManager::isSliderArea(int16_t x, int16_t y) {
    return (y > 380); // Bottom area in Rotation 2
}
#pragma once

// I2C Bus (Audio Codecs & Touch)
#define CUSTOM_I2C_SDA 47
#define CUSTOM_I2C_SCL 48

// I2S Bus (Mic & Speaker)
#define CUSTOM_I2S_BCLK 16
#define CUSTOM_I2S_WS   7
#define CUSTOM_I2S_DIN  15 // ESP32 receives from ES7210
#define CUSTOM_I2S_DOUT 6  // ESP32 sends to ES8311
#define CUSTOM_I2S_MCLK 5  // Master Clock

// Display Pins
#define CUSTOM_LCD_BLK  4
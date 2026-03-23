#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Bus_RGB       _bus_instance;
  lgfx::Panel_ST7701  _panel_instance;
  lgfx::Touch_GT911   _touch_instance;
  lgfx::Light_PWM     _light_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 480;
      cfg.memory_height = 480;
      cfg.panel_width   = 480;
      cfg.panel_height  = 480;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _panel_instance.config_detail();
      cfg.use_psram = 1;
      cfg.pin_cs    = 39;
      cfg.pin_sclk  = 48;
      cfg.pin_mosi  = 47;
      _panel_instance.config_detail(cfg);
    }

    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      cfg.pin_d0  = 10; // B0
      cfg.pin_d1  = 11; // B1
      cfg.pin_d2  = 12; // B2
      cfg.pin_d3  = 13; // B3
      cfg.pin_d4  = 14; // B4
      cfg.pin_d5  = 21; // G0
      cfg.pin_d6  = 8;  // G1
      cfg.pin_d7  = 18; // G2
      cfg.pin_d8  = 45; // G3
      cfg.pin_d9  = 38; // G4
      cfg.pin_d10 = 39; // G5
      cfg.pin_d11 = 40; // R0
      cfg.pin_d12 = 41; // R1
      cfg.pin_d13 = 42; // R2
      cfg.pin_d14 = 2;  // R3
      cfg.pin_d15 = 1;  // R4

      cfg.pin_henable = 17; // DE
      cfg.pin_vsync   = 3;
      cfg.pin_hsync   = 46;
      cfg.pin_pclk    = 9;
      cfg.freq_write  = 16000000;

      cfg.hsync_polarity    = 1;
      cfg.hsync_front_porch = 10;
      cfg.hsync_pulse_width = 8;
      cfg.hsync_back_porch  = 50;
      
      cfg.vsync_polarity    = 1;
      cfg.vsync_front_porch = 10;
      cfg.vsync_pulse_width = 8;
      cfg.vsync_back_porch  = 20;

      cfg.pclk_active_neg   = 1;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = 479;
      cfg.y_min      = 0;
      cfg.y_max      = 479;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port   = 0;
      cfg.pin_int    = -1;
      cfg.pin_rst    = -1;
      cfg.pin_sda    = 47;
      cfg.pin_scl    = 48;
      cfg.freq       = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

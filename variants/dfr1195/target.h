#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/ESP32Board.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>

#ifdef DISPLAY_CLASS
  #include <helpers/ui/ST7735Display.h>
  #include <helpers/ui/MomentaryButton.h>

// ST7735Display::turnOff() releases the panel pins, and its turnOn() never restores them.
class DFR1195Display : public ST7735Display {
public:
  void turnOn() override {
    if (!isOn()) {
      pinMode(PIN_TFT_RST, OUTPUT);
      pinMode(PIN_TFT_CS, OUTPUT);
      pinMode(PIN_TFT_DC, OUTPUT);
      pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
      // begin() returns early while the bus is still open, so close it to re-attach the pads.
      SPI1.end();
      SPI1.begin(PIN_TFT_SCL, -1 /* miso */, PIN_TFT_SDA, -1);
    }
    ST7735Display::turnOn();
  }
};
#endif

class DFR1195Board : public ESP32Board {
public:
  void begin();

  const char* getManufacturerName() const override {
    return "DFR1195";
  }
};

extern DFR1195Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern SensorManager sensors;

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();

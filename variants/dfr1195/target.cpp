#include <Arduino.h>
#include "target.h"

DFR1195Board board;

void DFR1195Board::begin() {
  ESP32Board::begin();

#if defined(DISPLAY_CLASS)
  // Must happen here: setup() calls display.begin() before radio_init(), and the panel
  // cannot accept its init sequence until this rail is up.
  #if defined(PIN_TFT_VDD_CTL) && (PIN_TFT_VDD_CTL >= 0)
    pinMode(PIN_TFT_VDD_CTL, OUTPUT);
    #if defined(PIN_TFT_VDD_CTL_ACTIVE)
      digitalWrite(PIN_TFT_VDD_CTL, PIN_TFT_VDD_CTL_ACTIVE);
    #else
      digitalWrite(PIN_TFT_VDD_CTL, HIGH);
    #endif
    delay(50);
  #endif

  #if defined(ST7735_SPI_HZ)
    // Owned by ST7735Display.cpp; its 40MHz default is above the ST7735S write-cycle rating.
    extern SPISettings _spiSettings;
    _spiSettings = SPISettings(ST7735_SPI_HZ, MSBFIRST, SPI_MODE0);
  #endif
#endif
}

SPIClass dfr1195_tft_spi(HSPI);

#if defined(P_LORA_SCLK)
  // The LCD owns HSPI, so the radio needs the other host: one host cannot serve both pin sets.
  static SPIClass spi(FSPI);
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
  // GPIO18 has an external 10K pull-up and the switch shorts to GND, so the button is active LOW.
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true, true, false);
#endif

#ifndef LORA_CR
  #define LORA_CR      5
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);

  pinMode(PIN_STATUS_LED, OUTPUT);

#ifdef SX126X_DIO3_TCXO_VOLTAGE
  float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif

#if defined(P_LORA_SCLK)
  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
#endif

  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    return false;
  }

  radio.setCRC(1);

#if defined(SX126X_RXEN) || defined(SX126X_TXEN)
  // The PE4259 takes a complementary pair: DIO2 drives CTRL, this drives ~CTRL. There is no TXEN net.
  #ifndef SX126X_RXEN
    #define SX126X_RXEN RADIOLIB_NC
  #endif
  #ifndef SX126X_TXEN
    #define SX126X_TXEN RADIOLIB_NC
  #endif
  radio.setRfSwitchPins(SX126X_RXEN, SX126X_TXEN);
#endif

#ifdef SX126X_CURRENT_LIMIT
  radio.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif
#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif
#ifdef SX126X_RX_BOOSTED_GAIN
  radio.setRxBoostedGainMode(SX126X_RX_BOOSTED_GAIN);
#endif

#ifdef RXPS_ENABLED
  radio.setRxPowerSaving(true);
#endif

  return true;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);
}

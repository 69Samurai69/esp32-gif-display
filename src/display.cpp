#include "display.h"
#include "config.h"

// Definicao do objeto global declarado em display.h
TFT_eSPI tft = TFT_eSPI();

namespace Display {

void begin() {
  tft.init();
  tft.setRotation(Config::rotation);
  tft.fillScreen(TFT_BLACK);

  // Backlight com PWM real (a placa controla o brilho pelo GPIO 4).
  ledcSetup(Config::BL_PWM_CHANNEL, Config::BL_PWM_FREQ, Config::BL_PWM_BITS);
  ledcAttachPin(Config::PIN_BACKLIGHT, Config::BL_PWM_CHANNEL);
  setBacklight(Config::brightness);
}

void setBacklight(uint8_t value) {
  ledcWrite(Config::BL_PWM_CHANNEL, value);
}

void setRotation(uint8_t r) {
  tft.setRotation(r & 0x03);
  tft.fillScreen(TFT_BLACK);
}

void showMessage(const String& title, const String& subtitle) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);  // centro
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  const int16_t cx = tft.width() / 2;
  const int16_t cy = tft.height() / 2;

  tft.setTextSize(1);
  tft.drawString(title, cx, subtitle.isEmpty() ? cy : cy - 12, 4);
  if (!subtitle.isEmpty()) {
    tft.drawString(subtitle, cx, cy + 14, 2);
  }
}

}  // namespace Display

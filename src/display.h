#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Objeto global do display, compartilhado com o callback GIFDraw.
extern TFT_eSPI tft;

namespace Display {

// Inicializa o painel ST7789 e o PWM de backlight.
void begin();

// Ajusta o brilho via PWM (0-255).
void setBacklight(uint8_t value);

// Muda a rotacao do painel em tempo real (0-3).
void setRotation(uint8_t r);

// Mostra uma mensagem simples de status (2 linhas) centralizada.
void showMessage(const String& title, const String& subtitle = "");

}  // namespace Display

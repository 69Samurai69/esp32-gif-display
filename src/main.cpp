// ============================================================================
//  GIF Display - firmware autonomo para TTGO T-Display ESP32
//  Player de GIF da flash (LittleFS) + WiFi (portal cativo) + servidor web
//  de gestao e configuracao da tela.
// ============================================================================
#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "storage.h"
#include "buttons.h"
#include "player.h"
#include "net.h"
#include "web.h"

static void resumePlayback() {
  if (Player::playlistSize() > 0) {
    Player::playCurrent();
  } else {
    Display::showMessage("Sem GIFs", "envie p/ /gifs");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[GIF-Display] Iniciando...");

  Display::begin();
  Display::showMessage("GIF Display", "iniciando...");

  Buttons::begin();

  if (!Storage::begin()) {
    Display::showMessage("Erro", "LittleFS falhou");
    return;
  }

  // Carrega as preferencias salvas e aplica brilho/rotacao.
  Config::load();
  Display::setRotation(Config::rotation);
  Display::setBacklight(Config::brightness);

  // WiFi: conecta com a rede salva ou abre o portal de configuracao.
  // (bloqueia enquanto o portal estiver aberto)
  Net::begin();
  if (Net::isConnected()) {
    Display::showMessage("WiFi OK", Net::ip());
    Web::begin();
    delay(1200);
  }

  Player::begin();
  Serial.printf("[GIF-Display] %u GIF(s) na playlist\n",
                (unsigned)Player::playlistSize());
  resumePlayback();
}

void loop() {
  Buttons::update();

  // Long-press no botao "anterior" (GPIO 0) reabre o portal de WiFi.
  if (Buttons::prevLongPressed()) {
    Net::startPortal();
    if (Net::isConnected()) {
      Display::showMessage("WiFi OK", Net::ip());
      Web::begin();
      delay(1200);
    }
    resumePlayback();
    return;
  }

  if (Buttons::nextPressed()) Player::next();
  if (Buttons::prevPressed()) Player::prev();

  Web::loop();
  Player::update();
}

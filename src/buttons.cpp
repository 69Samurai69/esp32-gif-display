#include "buttons.h"
#include "config.h"

namespace Buttons {
namespace {

constexpr unsigned long DEBOUNCE_MS = 40;
constexpr unsigned long LONG_MS     = 1500;  // limite para pressao longa

struct Button {
  uint8_t       pin;
  bool          pressed   = false;  // estado debounced (true = apertado)
  bool          lastRead  = false;  // ultima leitura crua (true = apertado)
  unsigned long lastChange = 0;
  unsigned long pressStart = 0;
  bool          longFired = false;
  bool          clickEdge = false;  // clique curto neste ciclo
  bool          longEdge  = false;  // pressao longa neste ciclo
  explicit Button(uint8_t p) : pin(p) {}
};

Button btnNext{Config::PIN_BUTTON_NEXT};
Button btnPrev{Config::PIN_BUTTON_PREV};

void poll(Button& b) {
  b.clickEdge = false;
  b.longEdge  = false;

  const bool reading = (digitalRead(b.pin) == LOW);  // pressionado = LOW

  if (reading != b.lastRead) {
    b.lastChange = millis();
    b.lastRead   = reading;
  }

  // Mudanca de estado estavel (passou o debounce).
  if ((millis() - b.lastChange) > DEBOUNCE_MS && reading != b.pressed) {
    b.pressed = reading;
    if (b.pressed) {                 // borda de descida: apertou
      b.pressStart = millis();
      b.longFired  = false;
    } else {                         // soltou
      if (!b.longFired && (millis() - b.pressStart) < LONG_MS)
        b.clickEdge = true;          // clique curto registra na soltura
    }
  }

  // Pressao longa: dispara uma unica vez enquanto segurado.
  if (b.pressed && !b.longFired && (millis() - b.pressStart) >= LONG_MS) {
    b.longFired = true;
    b.longEdge  = true;
  }
}

}  // namespace

void begin() {
  // GPIO 0 tem pull-up interno; GPIO 35 e input-only e usa pull-up externo
  // da propria placa, entao INPUT basta.
  pinMode(Config::PIN_BUTTON_PREV, INPUT_PULLUP);
  pinMode(Config::PIN_BUTTON_NEXT, INPUT);
}

void update() {
  poll(btnNext);
  poll(btnPrev);
}

bool nextPressed()     { return btnNext.clickEdge; }
bool prevPressed()     { return btnPrev.clickEdge; }
bool nextLongPressed() { return btnNext.longEdge; }
bool prevLongPressed() { return btnPrev.longEdge; }

}  // namespace Buttons

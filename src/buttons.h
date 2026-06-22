#pragma once
#include <Arduino.h>

// Leitura com debounce dos dois botoes fisicos da T-Display.
// Clique curto dispara na soltura; pressao longa dispara ao cruzar o limite.
namespace Buttons {

void begin();
void update();              // chamar a cada loop()

bool nextPressed();         // clique curto no botao "proximo" (GPIO 35)
bool prevPressed();         // clique curto no botao "anterior" (GPIO 0)

bool nextLongPressed();     // pressao longa no GPIO 35
bool prevLongPressed();     // pressao longa no GPIO 0 (abre config WiFi)

}  // namespace Buttons

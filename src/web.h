#pragma once
#include <Arduino.h>

// Servidor web de gerenciamento (porta 80). Lista/envia/apaga GIFs e
// controla a reproducao. Disponivel quando ha conexao WiFi.
namespace Web {

void begin();   // sobe o servidor (idempotente)
void loop();    // chamar a cada loop() para atender clientes

}  // namespace Web

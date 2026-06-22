#pragma once
#include <Arduino.h>
#include <vector>

namespace Storage {

// Monta o LittleFS (formata no 1o boot se necessario) e garante /gifs.
bool begin();

// Lista os arquivos .gif dentro de Config::GIF_DIR (caminhos completos).
std::vector<String> listGifs();

}  // namespace Storage

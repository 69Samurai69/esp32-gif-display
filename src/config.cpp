#include "config.h"
#include <FS.h>
#include <LittleFS.h>

using fs::File;

namespace Config {

// Valores padrao de fabrica (sobrescritos por load() se houver /config.cfg).
uint8_t  brightness    = 160;
uint8_t  rotation      = 1;  // 1/3 = paisagem (240x135); 0/2 = retrato (135x240)
uint32_t autoAdvanceMs = 0;
bool     loopGif       = true;

namespace {
constexpr const char* CFG_PATH = "/config.cfg";

// Aplica uma linha "chave=valor".
void applyKV(const String& key, const String& val) {
  if      (key == "brightness")    brightness    = (uint8_t)val.toInt();
  else if (key == "rotation")      rotation      = (uint8_t)(val.toInt() & 0x03);
  else if (key == "autoAdvanceMs") autoAdvanceMs = (uint32_t)val.toInt();
  else if (key == "loop")          loopGif       = (val.toInt() != 0);
}
}  // namespace

void load() {
  if (!LittleFS.exists(CFG_PATH)) return;
  File f = LittleFS.open(CFG_PATH, "r");
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    int eq = line.indexOf('=');
    if (eq > 0) applyKV(line.substring(0, eq), line.substring(eq + 1));
  }
  f.close();
}

void save() {
  File f = LittleFS.open(CFG_PATH, "w");
  if (!f) return;
  f.printf("brightness=%u\n", brightness);
  f.printf("rotation=%u\n", rotation);
  f.printf("autoAdvanceMs=%u\n", autoAdvanceMs);
  f.printf("loop=%u\n", loopGif ? 1 : 0);
  f.close();
}

}  // namespace Config

#include "storage.h"
#include "config.h"
#include <LittleFS.h>
#include <algorithm>

namespace Storage {

bool begin() {
  // true = formata automaticamente se a montagem falhar (1o boot).
  if (!LittleFS.begin(true)) {
    Serial.println("[Storage] Falha ao montar LittleFS");
    return false;
  }
  if (!LittleFS.exists(Config::GIF_DIR)) {
    LittleFS.mkdir(Config::GIF_DIR);
  }
  Serial.printf("[Storage] LittleFS: %u/%u bytes usados\n",
                (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
  return true;
}

std::vector<String> listGifs() {
  std::vector<String> out;
  File dir = LittleFS.open(Config::GIF_DIR);
  if (!dir || !dir.isDirectory()) return out;

  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    String name = f.name();          // pode vir so o nome ou caminho completo
    if (name.endsWith(".gif") || name.endsWith(".GIF")) {
      String full = name.startsWith("/") ? name
                                          : String(Config::GIF_DIR) + "/" + name;
      out.push_back(full);
    }
    f.close();
  }
  dir.close();

  std::sort(out.begin(), out.end());  // ordem estavel da playlist
  return out;
}

}  // namespace Storage

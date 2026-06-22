#include "web.h"
#include "config.h"
#include "display.h"
#include "player.h"
#include "storage.h"
#include "web_ui.h"
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>

using fs::File;

namespace Web {
namespace {

WebServer server(80);
bool      started = false;
File      uploadFile;

// Extrai apenas o nome base (sem componentes de diretorio) de um caminho.
String baseName(String name) {
  int s1 = name.lastIndexOf('/');
  int s2 = name.lastIndexOf('\\');
  int s  = max(s1, s2);
  if (s >= 0) name = name.substring(s + 1);
  return name;
}

bool isGif(const String& name) {
  String low = name;
  low.toLowerCase();
  return low.endsWith(".gif");
}

// Escape minimo para embutir em JSON.
String jsonEscape(const String& in) {
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  return out;
}

void handleList() {
  auto gifs = Storage::listGifs();
  String json = "{\"used\":" + String((uint32_t)LittleFS.usedBytes()) +
                ",\"total\":" + String((uint32_t)LittleFS.totalBytes()) +
                ",\"current\":" + String(Player::index()) + ",\"gifs\":[";
  for (size_t i = 0; i < gifs.size(); i++) {
    size_t sz = 0;
    File f = LittleFS.open(gifs[i], "r");
    if (f) { sz = f.size(); f.close(); }
    if (i) json += ",";
    json += "{\"name\":\"" + jsonEscape(baseName(gifs[i])) +
            "\",\"size\":" + String((uint32_t)sz) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// Recebe o arquivo em pedacos e grava direto no LittleFS.
String   uploadPath;     // caminho do arquivo em recepcao
uint32_t uploadBytes;    // bytes gravados nesta transferencia

void handleUploadData() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    String name = baseName(up.filename);
    if (!isGif(name)) name += ".gif";
    uploadPath = String(Config::GIF_DIR) + "/" + name;
    uploadBytes = 0;
    uploadFile = LittleFS.open(uploadPath, "w");
    Serial.printf("[Web] Upload: %s\n", uploadPath.c_str());
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadBytes += uploadFile.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    // Upload vazio/abortado: nao deixa arquivo de 0 byte para tras.
    if (uploadBytes == 0 && uploadPath.length()) {
      LittleFS.remove(uploadPath);
      Serial.println("[Web] Upload vazio descartado");
    }
  }
}

void handleDelete() {
  if (server.hasArg("name")) {
    String path = String(Config::GIF_DIR) + "/" + baseName(server.arg("name"));
    Player::stop();          // libera o arquivo caso ele esteja tocando
    LittleFS.remove(path);
    Player::reload();
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleGetConfig() {
  String json = "{\"brightness\":" + String(Config::brightness) +
                ",\"rotation\":" + String(Config::rotation) +
                ",\"autoAdvanceMs\":" + String(Config::autoAdvanceMs) +
                ",\"loop\":" + String(Config::loopGif ? "true" : "false") +
                ",\"screenW\":" + String(tft.width()) +
                ",\"screenH\":" + String(tft.height()) + "}";
  server.send(200, "application/json", json);
}

void handleSetConfig() {
  const uint8_t prevRotation = Config::rotation;

  if (server.hasArg("brightness"))
    Config::brightness = (uint8_t)constrain(server.arg("brightness").toInt(), 0, 255);
  if (server.hasArg("rotation"))
    Config::rotation = (uint8_t)(server.arg("rotation").toInt() & 0x03);
  if (server.hasArg("autoAdvanceMs"))
    Config::autoAdvanceMs = (uint32_t)server.arg("autoAdvanceMs").toInt();
  if (server.hasArg("loop"))
    Config::loopGif = (server.arg("loop").toInt() != 0);

  Display::setBacklight(Config::brightness);
  if (Config::rotation != prevRotation) {
    Display::setRotation(Config::rotation);
    Player::playCurrent();  // recentraliza na nova orientacao
  }
  Config::save();

  handleGetConfig();  // devolve o estado aplicado
}

}  // namespace

void begin() {
  if (started) return;

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/api/gifs", HTTP_GET, handleList);

  server.on(
      "/api/upload", HTTP_POST,
      []() {
        server.send(200, "application/json", "{\"ok\":true}");
        Player::reload();
      },
      handleUploadData);

  server.on("/api/delete", HTTP_POST, handleDelete);

  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSetConfig);

  server.on("/api/play", HTTP_POST, []() {
    if (server.hasArg("index")) Player::playIndex(server.arg("index").toInt());
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/next", HTTP_POST, []() {
    Player::next();
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/prev", HTTP_POST, []() {
    Player::prev();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound([]() { server.send(404, "text/plain", "404"); });

  server.begin();
  started = true;
  Serial.println("[Web] Servidor HTTP na porta 80");
}

void loop() {
  if (started) server.handleClient();
}

}  // namespace Web

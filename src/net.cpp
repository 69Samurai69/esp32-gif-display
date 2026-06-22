#include "net.h"
#include "display.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

namespace Net {
namespace {

constexpr char AP_NAME[]   = "GIF-Display";
constexpr char HOSTNAME[]  = "gifdisplay";
constexpr int  PORTAL_TIMEOUT_S = 180;

WiFiManager wm;

// Chamado pela lib quando o portal de configuracao (AP) sobe.
void onPortal(WiFiManager* /*m*/) {
  Display::showMessage("Configure WiFi", String("AP: ") + AP_NAME);
  Serial.printf("[Net] Portal aberto. Conecte ao AP '%s' e acesse 192.168.4.1\n",
                AP_NAME);
}

void startMDNS() {
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[Net] mDNS ativo: http://%s.local\n", HOSTNAME);
  }
}

}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  wm.setHostname(HOSTNAME);
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.setAPCallback(onPortal);
  // Apos salvar as credenciais no portal, reinicia para conectar limpo em STA
  // (evita o caso "configurei mas so funcionou apos reiniciar").
  wm.setSaveConfigCallback([]() {
    Serial.println("[Net] Credenciais salvas. Reiniciando...");
    Display::showMessage("WiFi salvo", "reiniciando...");
    delay(800);
    ESP.restart();
  });

  Display::showMessage("WiFi", "conectando...");

  // Bloqueia enquanto o portal estiver aberto (ou ate o timeout).
  if (wm.autoConnect(AP_NAME)) {
    Serial.printf("[Net] Conectado a '%s'  IP %s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    startMDNS();
  } else {
    Serial.println("[Net] Sem WiFi (timeout). Seguindo offline.");
  }
}

void startPortal() {
  Display::showMessage("Config WiFi", String("AP: ") + AP_NAME);
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  if (wm.startConfigPortal(AP_NAME)) {
    Serial.printf("[Net] Reconfigurado. IP %s\n", WiFi.localIP().toString().c_str());
    startMDNS();
  }
}

bool   isConnected() { return WiFi.status() == WL_CONNECTED; }
String ip()          { return WiFi.localIP().toString(); }
String hostname()    { return String(HOSTNAME) + ".local"; }

}  // namespace Net

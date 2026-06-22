# Roadmap — GIF Display autônomo (TTGO T-Display ESP32)

Placa: **LILYGO/TTGO T-Display ESP32** — 1.14" 135×240, ST7789, 2 botões
(GPIO 0 e 35), backlight GPIO 4, variante 16 MB de flash.

Objetivo: dispositivo **autossuficiente** que guarda os GIFs na própria flash
e oferece uma interface web (servida pelo ESP32) para configurar WiFi e
gerenciar os GIFs — sem depender de servidor externo.

## Arquitetura

- GIFs em **LittleFS** (~14 MB livres na variante de 16 MB).
- Player lê o GIF **em streaming** do arquivo (callbacks da lib AnimatedGIF),
  sem `malloc` do arquivo inteiro — corrige os vazamentos do sketch original.
- Web server **assíncrono** para não travar o player durante uploads.
- Configuração de WiFi via **portal cativo** (sem credenciais no código).

## Bibliotecas

| Função      | Lib                              |
|-------------|----------------------------------|
| Display     | `bodmer/TFT_eSPI`                |
| GIF         | `bitbank2/AnimatedGIF`           |
| Filesystem  | `LittleFS` (core ESP32)          |
| Web server  | `ESPAsyncWebServer` + `AsyncTCP` |
| WiFi config | `tzapu/WiFiManager`              |
| Config JSON | `bblanchon/ArduinoJson`          |
| OTA         | `ayushsharma82/ElegantOTA`       |

## Fases

### Fase 0 — Higiene e refactor  ✅
- Migração para PlatformIO com pinagem via `build_flags`.
- Modularização (`display`, `player`, `storage`, `buttons`, `config`).
- Backlight no GPIO **4** com PWM real (`ledc`).
- Sem credenciais hardcoded.

### Fase 1 — Player a partir da flash  ✅
- AnimatedGIF lendo do LittleFS em streaming.
- Centralização do GIF na tela; recorte de excedente.
- Player não-bloqueante (1 frame por `update()`), loop e auto-advance.
- Botões: GPIO 35 = próximo, GPIO 0 = anterior (debounce).

### Fase 2 — Configuração de WiFi  ✅
- `WiFiManager`: 1º boot sobe AP `GIF-Display`; usuário escolhe a rede.
- `WiFi.setHostname` + mDNS → `http://gifdisplay.local`.
- Long-press no GPIO 0 reabre o portal de configuração.
- Tela mostra IP/status da conexão.

### Fase 3 — Interface web de gestão  ✅
- Partição custom `partitions_16mb.csv` (2× app de 3 MB + ~9,8 MB LittleFS);
  uso do app caiu de ~76% para ~32%.
- `WebServer` síncrono (já incluso via WiFiManager) em vez do
  ESPAsyncWebServer — mais leve e sem dependência extra.
- UI embutida em PROGMEM (`web_ui.h`), servida em `/`.
- Endpoints: `/api/gifs` (listar + uso), `/api/upload` (multipart),
  `/api/delete`, `/api/play`, `/api/next`, `/api/prev`.
- Barra de uso da flash; aviso de dimensão recomendada (135×240).

> Reordenar a playlist ficou de fora (ordem ainda é alfabética); pode entrar
> na Fase 4 junto da persistência.

### Fase 4 — Settings persistentes  ✅ (parcial)
- Persistência em `/config.cfg` no LittleFS (formato chave=valor, sem libs).
- Brilho, rotação, auto-advance e loop, aplicados em tempo real.
- API `GET/POST /api/config`; carregados no boot (`Config::load()`).
- Falta: editar settings pela UI web embutida e reordenar playlist.

### App de PC (tools/esp32gif)  ✅
- Python (GUI Tkinter + CLI) que fala com a API HTTP do ESP32.
- **Redimensiona o GIF antes do upload** (Pillow): stretch/fit/fill p/ 240×135.
- Gerencia playlist (listar/enviar/apagar/play) e a configuração da tela.

### Fase 5 — Polimento
- OTA pelo navegador (`ElegantOTA`).
- Tela de status na placa (IP, nº de GIFs, modo atual).
- (Opcional) fonte remota alternativa: buscar GIF de um servidor HTTP.

## Limitações conhecidas

- Tela 135×240: GIFs grandes são recortados. Ideal preparar no tamanho certo.
- Sem escala on-device (custo de CPU/RAM). Redimensionamento fica no envio.
- RAM do ESP32 (~200–300 KB livres): tocar 1 GIF por vez, em streaming.

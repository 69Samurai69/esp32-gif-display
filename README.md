# GIF Display — TTGO T-Display ESP32

Exibidor de GIFs **autônomo e configurável pela rede** para a placa
**LILYGO/TTGO T-Display ESP32** (1.14" 135×240, ST7789). Os GIFs ficam na
flash interna do ESP32 e tocam em loop; você gerencia tudo por uma **interface
web** servida pela própria placa ou por um **app de PC** (GUI + CLI) que ainda
**redimensiona os GIFs antes do upload**.

## Recursos

- ▶️ Reproduz GIFs animados a partir da flash (LittleFS), em streaming.
- 📶 Configuração de WiFi via **portal cativo** (sem senha no código).
- 🌐 **Interface web** embutida (`http://gifdisplay.local`): enviar, apagar,
  trocar e configurar — sem recompilar.
- 💻 **App de PC** ([tools/esp32gif](tools/esp32gif)): GUI + CLI que
  **redimensiona o GIF** (Pillow) para a tela antes de enviar.
- 🔆 Ajuste de **brilho** e **rotação** em tempo real, persistidos entre reboots.
- 🔁 Modo de reprodução: **travar** num GIF ou **trocar periodicamente**.
- 🔘 Dois botões físicos para navegar; long-press reabre o portal de WiFi.

## Hardware

| Item            | Pino                                    |
|-----------------|-----------------------------------------|
| Display SPI     | MOSI 19, SCLK 18, CS 5, DC 16, RST 23   |
| Backlight       | GPIO 4 (PWM)                            |
| Botão "próximo" | GPIO 35                                 |
| Botão "anterior"| GPIO 0                                  |

Placa de **16 MB** de flash (partição custom em [partitions_16mb.csv](partitions_16mb.csv)
dá ~9,8 MB de LittleFS para os GIFs).

## Build e gravação

Requisitos: [PlatformIO](https://platformio.org/) (CLI ou extensão do VS Code).

```bash
# Compilar e gravar o firmware
pio run -t upload

# (opcional) carga inicial de GIFs de ./data/gifs para a flash
pio run -t uploadfs

# Log serial
pio device monitor
```

A tela em paisagem é **240×135**: **redimensione os GIFs** para esse tamanho
(o app de PC faz isso automaticamente). GIFs maiores são recortados/centralizados.

### Configurar o WiFi

No 1º boot (ou se a rede salva falhar), a placa sobe o ponto de acesso
**`GIF-Display`**. Conecte o celular nele e abra `http://192.168.4.1` para
escolher sua rede. Depois conecta sozinho e fica em `http://gifdisplay.local`
(mDNS). Para reconfigurar, **segure o botão da esquerda (GPIO 0) ~1,5 s**.

### Gerenciar pelo navegador

Com a placa no WiFi, acesse **`http://gifdisplay.local`** (ou o IP na tela):
enviar GIFs (arrastar e soltar), apagar, trocar o atual, ajustar brilho/rotação
e o modo de reprodução.

### App de PC (com redimensionamento)

Veja [tools/esp32gif/README.md](tools/esp32gif/README.md). Resumo:

```bash
cd tools/esp32gif
pip install -r requirements.txt
python -m esp32gif --host gifdisplay.local gui      # janela desktop
# ou empacote um .exe Windows: build_exe.bat
```

## Estrutura

```
platformio.ini      Build + pinagem da T-Display (TFT_eSPI via build_flags)
partitions_16mb.csv Partição: 2× app de 3 MB + ~9,8 MB LittleFS
src/
  main.cpp          setup()/loop()
  config.*          preferências (brilho, rotação, modo) em /config.cfg
  display.*         init ST7789 + backlight PWM
  storage.*         LittleFS + listagem dos GIFs
  player.*          player não-bloqueante, GIF em streaming da flash
  gifdraw.*         desenho linha-a-linha (AnimatedGIF)
  buttons.*         botões com debounce (clique curto / longo)
  net.*             WiFiManager (portal cativo) + mDNS
  web.* / web_ui.h  servidor HTTP + interface web embutida
data/gifs/          GIFs para a carga inicial via `uploadfs`
tools/esp32gif/     app de PC (Python): GUI + CLI + redimensionamento
```

Plano de evolução e fases em [ROADMAP.md](ROADMAP.md).

## Créditos

- [bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — driver do display.
- [bitbank2/AnimatedGIF](https://github.com/bitbank2/AnimatedGIF) — decodificação
  de GIF; o callback `GIFDraw` é adaptado do exemplo oficial da biblioteca.
- [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager) — portal cativo.

## Licença

[MIT](LICENSE).

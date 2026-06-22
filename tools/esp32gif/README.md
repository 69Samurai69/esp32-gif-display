# esp32gif — gerenciador de PC para o GIF Display

App em Python que fala com o ESP32 pela rede para **enviar GIFs
(redimensionando antes do upload)**, gerenciar a playlist e **configurar a
tela** (brilho, rotação, auto-avanço, loop). Tem **GUI** (janela desktop) e
**CLI**.

## Instalação

```bash
cd tools/esp32gif
pip install -r requirements.txt
```

Requisitos: Python 3.9+, [Pillow](https://python-pillow.org/) e
[requests](https://requests.readthedocs.io/).

## Endereço do dispositivo

Use o hostname mDNS `gifdisplay.local` ou o IP mostrado na tela do ESP32.
Pode definir um padrão na variável de ambiente `ESP32GIF_HOST`.

> No Windows, `gifdisplay.local` exige o serviço Bonjour/mDNS. Se não
> resolver, use o IP (ex.: `192.168.0.50`).

O **último host usado fica salvo** em `~/.esp32gif.json` e vira o padrão na
próxima abertura (tanto na GUI quanto na CLI). A ordem de prioridade é:
`--host`/campo digitado → variável `ESP32GIF_HOST` → último salvo →
`gifdisplay.local`.

## GUI

```bash
python -m esp32gif --host gifdisplay.local gui
```

Janela com: lista de GIFs do dispositivo, botões tocar/apagar/anterior/próximo,
envio com escolha de modo de ajuste, e painel de configuração da tela
(brilho, rotação, auto-avanço, loop).

## Executável Windows (sem precisar de Python)

Para gerar um `.exe` que abre a GUI com 2 cliques:

```bash
pip install -r requirements.txt pyinstaller
build_exe.bat            # ou: python -m PyInstaller --onefile --windowed --name "GIF-Display" --paths . gui_launcher.py
```

O executável fica em `dist\GIF-Display.exe` (~18 MB, autocontido). Distribua
só esse arquivo. Ele abre a janela; o endereço do ESP32 é digitado na própria
janela (ou passe `GIF-Display.exe --host gifdisplay.local`).

## CLI

```bash
# estado da tela + uso da flash
python -m esp32gif --host gifdisplay.local info

# listar GIFs
python -m esp32gif --host gifdisplay.local ls

# enviar (redimensiona para 240x135 por padrão; modo: stretch|fit|fill)
python -m esp32gif --host gifdisplay.local upload meu.gif
python -m esp32gif --host gifdisplay.local upload a.gif b.gif --mode fit
python -m esp32gif --host gifdisplay.local upload original.gif --no-resize

# apagar / tocar / navegar
python -m esp32gif --host gifdisplay.local rm meu.gif
python -m esp32gif --host gifdisplay.local play 0
python -m esp32gif --host gifdisplay.local next
python -m esp32gif --host gifdisplay.local prev

# configurar a tela
python -m esp32gif --host gifdisplay.local config --brightness 200 --rotation 1

# modo de reproducao:
python -m esp32gif --host gifdisplay.local config --lock        # trava no GIF atual
python -m esp32gif --host gifdisplay.local config --every 10    # troca a cada 10 s
```

### Modo de reprodução

- **Travar**: fica em um GIF só (loop infinito). Na GUI/web escolha "Travar no
  GIF atual"; na CLI use `--lock`. Para fixar um GIF específico, primeiro
  `play <indice>` (ou clique nele) e depois trave.
- **Trocar periodicamente**: passa para o próximo GIF a cada N segundos.
  GUI/web "Trocar periodicamente"; CLI `--every N`.

## Modos de redimensionamento

| Modo      | Comportamento                                            |
|-----------|----------------------------------------------------------|
| `stretch` | estica para 240×135 exatos (pode distorcer) — **padrão** |
| `fit`     | mantém proporção, barras pretas (letterbox)              |
| `fill`    | mantém proporção e corta o excesso (cover)               |

Os frames são achatados sobre fundo preto (a tela é preta) e requantizados
para 256 cores — evita artefatos de transparência no decodificador do ESP32.

## Teste local (sem ESP32)

```bash
python _selftest.py   # valida o redimensionamento nos 3 modos
```

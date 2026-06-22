#include "player.h"
#include "config.h"
#include "display.h"
#include "storage.h"
#include "gifdraw.h"
#include <AnimatedGIF.h>
#include <FS.h>
#include <LittleFS.h>

using fs::File;  // a classe File vive em namespace fs; garante visibilidade

namespace Player {
namespace {

AnimatedGIF          gif;
std::vector<String>  playlist;
int                  currentIndex = 0;
bool                 gifOpen      = false;
unsigned long        nextFrameAt  = 0;   // millis() do proximo frame
unsigned long        startedAt    = 0;   // inicio do GIF (para auto-advance)

File gifFile;  // um GIF aberto por vez

// --- Callbacks de arquivo para a lib AnimatedGIF (LittleFS) ----------------
void* gifOpenFile(const char* fname, int32_t* pSize) {
  gifFile = LittleFS.open(fname, "r");
  if (!gifFile) return nullptr;
  *pSize = gifFile.size();
  return (void*)&gifFile;
}

void gifCloseFile(void* pHandle) {
  File* f = static_cast<File*>(pHandle);
  if (f && *f) f->close();
}

int32_t gifReadFile(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  File* f = static_cast<File*>(pFile->fHandle);
  int32_t want = iLen;
  if ((pFile->iSize - pFile->iPos) < iLen) want = pFile->iSize - pFile->iPos - 1;
  if (want <= 0) return 0;
  int32_t got = (int32_t)f->read(pBuf, want);
  pFile->iPos = f->position();
  return got;
}

int32_t gifSeekFile(GIFFILE* pFile, int32_t iPosition) {
  File* f = static_cast<File*>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = iPosition;
  return iPosition;
}

void closeIfOpen() {
  if (gifOpen) {
    gif.close();
    gifOpen = false;
  }
}

}  // namespace

void begin() {
  gif.begin(BIG_ENDIAN_PIXELS);
  playlist = Storage::listGifs();
  currentIndex = 0;
}

size_t playlistSize() { return playlist.size(); }

void playCurrent() {
  closeIfOpen();
  if (playlist.empty()) return;
  if (currentIndex < 0) currentIndex = playlist.size() - 1;
  if (currentIndex >= (int)playlist.size()) currentIndex = 0;

  const char* path = playlist[currentIndex].c_str();
  Serial.printf("[Player] Abrindo %s\n", path);

  if (gif.open(path, gifOpenFile, gifCloseFile, gifReadFile, gifSeekFile, GIFDraw)) {
    gifOpen = true;
    const int cw = gif.getCanvasWidth();
    const int ch = gif.getCanvasHeight();
    Serial.printf("[Player] OK: canvas %dx%d (tela %dx%d)\n",
                  cw, ch, tft.width(), tft.height());
    // Centraliza o GIF na tela.
    gGifOffsetX = (tft.width()  - cw) / 2;
    gGifOffsetY = (tft.height() - ch) / 2;
    if (gGifOffsetX < 0) gGifOffsetX = 0;
    if (gGifOffsetY < 0) gGifOffsetY = 0;

    tft.fillScreen(TFT_BLACK);
    nextFrameAt = millis();
    startedAt   = millis();
  } else {
    // open() falhou e NAO chama o close callback: fecha o arquivo na mao,
    // senao o handle vaza e o LittleFS nao consegue apaga-lo depois.
    if (gifFile) gifFile.close();
    const int err = gif.getLastError();
    Serial.printf("[Player] Falha ao abrir GIF (erro %d)\n", err);
    Display::showMessage("GIF erro " + String(err), path);
  }
}

void next() {
  currentIndex++;
  playCurrent();
}

void prev() {
  currentIndex--;
  playCurrent();
}

void stop() {
  closeIfOpen();
  if (gifFile) gifFile.close();  // garante liberar mesmo apos open() falho
}

int index() { return currentIndex; }

void playIndex(int i) {
  currentIndex = i;
  playCurrent();
}

void reload() {
  // Preserva o arquivo que esta tocando, se ele continuar existindo.
  const String cur = (currentIndex >= 0 && currentIndex < (int)playlist.size())
                         ? playlist[currentIndex]
                         : String();
  playlist = Storage::listGifs();

  if (playlist.empty()) {
    closeIfOpen();
    currentIndex = 0;
    return;
  }

  for (size_t i = 0; i < playlist.size(); i++) {
    if (playlist[i] == cur) {
      currentIndex = (int)i;
      if (!gifOpen) playCurrent();  // recomeca so se nada estava tocando
      return;
    }
  }
  // O arquivo atual sumiu (delete): ajusta o indice e toca o novo atual.
  if (currentIndex >= (int)playlist.size()) currentIndex = playlist.size() - 1;
  if (currentIndex < 0) currentIndex = 0;
  playCurrent();
}

void update() {
  if (!gifOpen) return;

  const unsigned long now = millis();

  // Avanco automatico de GIF, se habilitado.
  if (Config::autoAdvanceMs > 0 && (now - startedAt) >= Config::autoAdvanceMs) {
    next();
    return;
  }

  if (now < nextFrameAt) return;  // ainda nao e hora do proximo frame

  int frameDelayMs = 0;
  // O GIFDraw faz varios setAddrWindow/pushPixels por frame: precisam estar
  // numa unica transacao SPI (startWrite/endWrite), senao nada e desenhado.
  tft.startWrite();
  int rc = gif.playFrame(false, &frameDelayMs);  // false = nao bloqueia
  tft.endWrite();

  // Muitos GIFs guardam delay 0/minusculo contando que o player aplique um
  // minimo (navegadores: <=10ms vira 100ms). Sem isso fica ultra rapido.
  if (frameDelayMs <= 10) frameDelayMs = 100;
  nextFrameAt = now + frameDelayMs;

  if (rc == 0) {
    const int err = gif.getLastError();
    if (err != 0) Serial.printf("[Player] Erro de decodificacao: %d\n", err);
    // Fim da animacao (ou erro): recomeca ou avanca.
    // NAO resetar startedAt aqui: o timer de auto-troca conta desde que o
    // GIF entrou (em playCurrent), independente de quantas vezes ele loopa.
    if (Config::loopGif) {
      gif.reset();              // recomeca do primeiro frame
    } else if (playlist.size() > 1) {
      next();
    }
  }
}

}  // namespace Player

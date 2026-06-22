#include "gifdraw.h"
#include "display.h"

// Buffer de uma linha. ST7789 da T-Display tem no maximo 240px de largura.
#define MAX_LINE_WIDTH 240
static uint16_t lineBuffer[MAX_LINE_WIDTH];

int gGifOffsetX = 0;
int gGifOffsetY = 0;

// Adaptado do exemplo oficial bitbank2/AnimatedGIF para TFT_eSPI,
// acrescentando o deslocamento de centralizacao (gGifOffsetX/Y).
void GIFDraw(GIFDRAW* pDraw) {
  uint8_t* s;
  uint16_t* d;
  uint16_t* usPalette = pDraw->pPalette;
  int x, y, iWidth, iCount;

  iWidth = pDraw->iWidth;
  if (iWidth > MAX_LINE_WIDTH) iWidth = MAX_LINE_WIDTH;

  y = pDraw->iY + pDraw->y + gGifOffsetY;  // linha absoluta na tela
  const int xStart = pDraw->iX + gGifOffsetX;

  // Recorta o que cai fora da tela.
  if (y < 0 || y >= tft.height() || xStart >= tft.width() || iWidth < 1) return;

  s = pDraw->pPixels;

  // Disposal method 2: pixels transparentes viram a cor de fundo.
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  if (pDraw->ucHasTransparency) {
    // Desenha em segmentos, pulando os pixels transparentes.
    const uint8_t ucTransparent = pDraw->ucTransparent;
    uint8_t* pEnd = s + iWidth;
    x = 0;
    iCount = 0;
    while (x < iWidth) {
      uint8_t c = ucTransparent - 1;
      d = lineBuffer;
      while (c != ucTransparent && s < pEnd && iCount < MAX_LINE_WIDTH) {
        c = *s++;
        if (c == ucTransparent) {
          s--;
        } else {
          *d++ = usPalette[c];
          iCount++;
        }
      }
      if (iCount) {
        tft.setAddrWindow(xStart + x, y, iCount, 1);
        tft.pushPixels(lineBuffer, iCount);
        x += iCount;
        iCount = 0;
      }
      // Pula a sequencia de transparentes.
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  } else {
    // Linha opaca: converte a paleta e empurra de uma vez.
    for (x = 0; x < iWidth; x++) lineBuffer[x] = usPalette[*s++];
    tft.setAddrWindow(xStart, y, iWidth, 1);
    tft.pushPixels(lineBuffer, iWidth);
  }
}

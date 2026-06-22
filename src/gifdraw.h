#pragma once
#include <AnimatedGIF.h>

// Deslocamentos para centralizar o GIF na tela. Definidos pelo Player
// sempre que um novo GIF e aberto (tela - canvas) / 2.
extern int gGifOffsetX;
extern int gGifOffsetY;

// Callback de desenho linha-a-linha chamado pela lib AnimatedGIF.
void GIFDraw(GIFDRAW* pDraw);

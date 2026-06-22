#pragma once
#include <Arduino.h>

// ============================================================================
//  Player de GIF nao-bloqueante.
//  Le o arquivo direto do LittleFS em streaming (sem carregar tudo na RAM)
//  e desenha um frame por chamada de update(), respeitando o delay do GIF.
// ============================================================================
namespace Player {

void   begin();          // carrega a playlist a partir do LittleFS
void   update();         // desenha frame quando chegar a hora (chamar no loop)

void   next();           // proximo GIF
void   prev();           // GIF anterior

size_t playlistSize();
void   playCurrent();    // (re)abre o GIF do indice atual

// --- Usados pela interface web (Fase 3) ------------------------------------
void   stop();           // fecha o GIF atual, liberando o arquivo no LittleFS
void   reload();         // recarrega a lista do LittleFS (apos upload/delete)
int    index();          // indice do GIF atual na playlist
void   playIndex(int i); // pula direto para um indice

}  // namespace Player

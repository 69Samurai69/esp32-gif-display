#pragma once
#include <Arduino.h>

// ============================================================================
//  Configuracoes globais do dispositivo.
//  Na Fase 4 estas variaveis passarao a ser carregadas/salvas em /config.json
//  no LittleFS. Por enquanto sao os valores padrao de fabrica.
// ============================================================================
namespace Config {

// --- Hardware (TTGO T-Display) ---------------------------------------------
constexpr uint8_t PIN_BUTTON_NEXT = 35;  // botao "direito" (input-only)
constexpr uint8_t PIN_BUTTON_PREV = 0;   // botao "esquerdo" (tem pull-up interno)
constexpr uint8_t PIN_BACKLIGHT   = 4;   // backlight real desta placa (nao o 32!)

// Canal/freq do PWM de backlight (LEDC)
constexpr uint8_t  BL_PWM_CHANNEL = 7;
constexpr uint32_t BL_PWM_FREQ    = 5000;
constexpr uint8_t  BL_PWM_BITS    = 8;

// --- Preferencias do usuario (mutaveis) ------------------------------------
extern uint8_t  brightness;      // 0-255
extern uint8_t  rotation;        // 0-3 (TFT_eSPI setRotation)
extern uint32_t autoAdvanceMs;   // 0 = nao avanca sozinho; >0 = intervalo
extern bool     loopGif;         // repetir o GIF atual ao terminar

// Caminho onde os GIFs ficam no LittleFS
constexpr const char* GIF_DIR = "/gifs";

// Persistencia das preferencias em /config.cfg (LittleFS).
void load();   // carrega do arquivo, se existir
void save();   // grava o estado atual

}  // namespace Config

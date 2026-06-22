#pragma once
#include <Arduino.h>

// ============================================================================
//  Conectividade WiFi com portal cativo (sem credenciais no codigo).
//  1o boot (ou se a rede salva falhar): sobe um AP "GIF-Display" com pagina
//  de configuracao. Depois de configurado, conecta sozinho e publica mDNS.
// ============================================================================
namespace Net {

void   begin();        // conecta com credenciais salvas ou abre o portal
void   startPortal();  // forca o portal de configuracao sob demanda (bloqueia)

bool   isConnected();
String ip();
String hostname();     // ex.: "gifdisplay.local"

}  // namespace Net

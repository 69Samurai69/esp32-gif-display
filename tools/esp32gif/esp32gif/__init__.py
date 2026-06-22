"""Ferramentas de PC para o GIF Display (ESP32 TTGO T-Display).

Conecta ao ESP32 via HTTP para enviar/gerenciar GIFs (redimensionando-os
antes do upload) e ajustar as configuracoes da tela.
"""
from .client import ESP32Client
from .resize import resize_gif, DEFAULT_SIZE

__all__ = ["ESP32Client", "resize_gif", "DEFAULT_SIZE"]
__version__ = "1.0.0"

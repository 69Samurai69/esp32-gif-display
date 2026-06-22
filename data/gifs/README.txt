Coloque aqui os arquivos .gif que serao gravados na flash do ESP32.

Para enviar para a placa:
    pio run -t uploadfs

Dicas:
- A tela e 135x240 (ST7789). GIFs maiores serao recortados/centralizados.
  Prepare/redimensione os GIFs para no maximo 135x240 antes de enviar.
- A variante de 16MB de flash comporta varios MB de GIFs.
- Os nomes definem a ordem da playlist (ordenacao alfabetica).

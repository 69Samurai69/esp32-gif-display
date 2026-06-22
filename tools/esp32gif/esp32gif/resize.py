"""Redimensionamento de GIFs (animados) para a tela do ESP32 antes do upload."""
from __future__ import annotations

from io import BytesIO
from typing import Union

from PIL import Image, ImageSequence

# Tamanho da tela da TTGO T-Display em paisagem (rotation 1/3).
DEFAULT_SIZE = (240, 135)

# Compatibilidade entre versoes do Pillow para o filtro de reamostragem.
try:
    _RESAMPLE = Image.Resampling.LANCZOS
except AttributeError:  # Pillow < 9.1
    _RESAMPLE = Image.LANCZOS


def _fit_frame(rgb: Image.Image, w: int, h: int, mode: str,
               bg: tuple[int, int, int]) -> Image.Image:
    """Ajusta um frame RGB a (w, h) conforme o modo escolhido."""
    if mode == "stretch":
        return rgb.resize((w, h), _RESAMPLE)

    if mode == "fit":  # mantem proporcao, barras pretas (letterbox)
        scaled = rgb.copy()
        scaled.thumbnail((w, h), _RESAMPLE)
        canvas = Image.new("RGB", (w, h), bg)
        canvas.paste(scaled, ((w - scaled.width) // 2, (h - scaled.height) // 2))
        return canvas

    if mode == "fill":  # mantem proporcao, corta o excesso (cover)
        scale = max(w / rgb.width, h / rgb.height)
        new = rgb.resize((max(1, round(rgb.width * scale)),
                          max(1, round(rgb.height * scale))), _RESAMPLE)
        left = (new.width - w) // 2
        top = (new.height - h) // 2
        return new.crop((left, top, left + w, top + h))

    raise ValueError(f"modo de redimensionamento invalido: {mode!r}")


def resize_gif(src: Union[str, bytes, bytearray],
               size: tuple[int, int] = DEFAULT_SIZE,
               mode: str = "stretch",
               bg: tuple[int, int, int] = (0, 0, 0)) -> bytes:
    """Redimensiona um GIF (animado ou nao) e devolve os bytes do novo GIF.

    - size: (largura, altura) destino. Padrao 240x135 (a tela do dispositivo).
    - mode: 'stretch' (estica), 'fit' (letterbox) ou 'fill' (corta).
    - bg:   cor de fundo usada para achatar transparencia e barras.

    Os frames sao achatados sobre `bg` (a tela e preta), o que evita
    artefatos de transparencia no decodificador do ESP32.
    """
    im = (Image.open(BytesIO(bytes(src)))
          if isinstance(src, (bytes, bytearray)) else Image.open(src))

    w, h = size
    frames: list[Image.Image] = []
    durations: list[int] = []

    for frame in ImageSequence.Iterator(im):
        rgba = frame.convert("RGBA")
        flat = Image.new("RGBA", rgba.size, bg + (255,))
        flat.alpha_composite(rgba)
        rgb = flat.convert("RGB")
        out = _fit_frame(rgb, w, h, mode, bg)
        # Quantiza para paleta de 256 cores (formato nativo do GIF).
        frames.append(out.convert("P", palette=Image.ADAPTIVE, colors=256))
        durations.append(int(frame.info.get("duration", 100)))

    loop = int(im.info.get("loop", 0))
    buf = BytesIO()
    frames[0].save(buf, format="GIF", save_all=True,
                   append_images=frames[1:], duration=durations,
                   loop=loop, disposal=2, optimize=True)
    return buf.getvalue()

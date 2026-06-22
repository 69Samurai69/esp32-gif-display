"""Teste local do redimensionamento (nao precisa do ESP32)."""
from io import BytesIO

from PIL import Image, ImageSequence

from esp32gif.resize import resize_gif


def count_frames(data: bytes) -> int:
    return sum(1 for _ in ImageSequence.Iterator(Image.open(BytesIO(data))))


def make_src() -> bytes:
    frames = [Image.new("RGB", (320, 240), c)
              for c in [(200, 30, 30), (30, 200, 30), (30, 30, 200)]]
    buf = BytesIO()
    frames[0].save(buf, format="GIF", save_all=True, append_images=frames[1:],
                   duration=120, loop=0)
    return buf.getvalue()


def main() -> None:
    src = make_src()
    print(f"original: {len(src)} bytes 320x240 frames={count_frames(src)}")
    for mode in ("stretch", "fit", "fill"):
        out = resize_gif(src, size=(240, 135), mode=mode)
        im = Image.open(BytesIO(out))
        assert im.size == (240, 135), im.size
        print(f"{mode:8s}-> {im.size} frames={count_frames(out)} "
              f"{len(out)} bytes loop={im.info.get('loop')}")
    print("OK")


if __name__ == "__main__":
    main()

"""Ponto de entrada do executavel da GUI (empacotado com PyInstaller)."""
import argparse

from esp32gif.gui import run


def main():
    p = argparse.ArgumentParser(description="GIF Display - janela desktop")
    p.add_argument("--host", default=None,
                   help="IP ou hostname do ESP32 (ex.: gifdisplay.local)")
    args = p.parse_args()
    run(host=args.host)


if __name__ == "__main__":
    main()

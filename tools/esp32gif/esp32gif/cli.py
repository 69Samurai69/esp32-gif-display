"""Interface de linha de comando do esp32gif."""
from __future__ import annotations

import argparse
import os
import sys

import requests

from .client import ESP32Client
from .prefs import default_host, set_saved_host
from .resize import DEFAULT_SIZE, resize_gif

DEFAULT_HOST = default_host()


def _human(n: int) -> str:
    return f"{n/1048576:.1f} MB" if n >= 1048576 else f"{n/1024:.0f} KB"


def cmd_info(c: ESP32Client, args) -> int:
    cfg = c.get_config()
    d = c.list_gifs()
    print(f"Tela: {cfg['screenW']}x{cfg['screenH']}  rotation={cfg['rotation']}  "
          f"brilho={cfg['brightness']}  loop={cfg['loop']}  "
          f"autoAdvanceMs={cfg['autoAdvanceMs']}")
    print(f"Flash: {_human(d['used'])} / {_human(d['total'])}  "
          f"({len(d['gifs'])} GIFs)")
    return 0


def cmd_ls(c: ESP32Client, args) -> int:
    d = c.list_gifs()
    cur = d.get("current", -1)
    for i, g in enumerate(d["gifs"]):
        mark = ">" if i == cur else " "
        print(f"{mark} [{i}] {g['name']}  ({_human(g['size'])})")
    return 0


def cmd_upload(c: ESP32Client, args) -> int:
    size = (args.width, args.height)
    for path in args.files:
        name = os.path.basename(path)
        if not name.lower().endswith(".gif"):
            name += ".gif"
        if args.no_resize:
            with open(path, "rb") as f:
                data = f.read()
            print(f"Enviando {name} (sem redimensionar, {_human(len(data))})...")
        else:
            data = resize_gif(path, size=size, mode=args.mode)
            print(f"Enviando {name} -> {size[0]}x{size[1]} ({args.mode}, "
                  f"{_human(len(data))})...")
        c.upload(name, data)
        print("  ok")
    return 0


def cmd_rm(c: ESP32Client, args) -> int:
    c.delete(args.name)
    print(f"removido: {args.name}")
    return 0


def cmd_play(c: ESP32Client, args) -> int:
    c.play(args.index)
    return 0


def cmd_next(c: ESP32Client, args) -> int:
    c.next()
    return 0


def cmd_prev(c: ESP32Client, args) -> int:
    c.prev()
    return 0


def cmd_config(c: ESP32Client, args) -> int:
    kw = {}
    if args.brightness is not None:
        kw["brightness"] = args.brightness
    if args.rotation is not None:
        kw["rotation"] = args.rotation
    # Modo de reproducao: --lock (trava) ou --every N (troca a cada N segundos).
    if args.lock:
        kw["autoAdvanceMs"] = 0
    elif args.every is not None:
        kw["autoAdvanceMs"] = max(1, args.every) * 1000
    elif args.auto_advance is not None:
        kw["autoAdvanceMs"] = args.auto_advance
    if args.loop is not None:
        kw["loop"] = 1 if args.loop else 0
    cfg = c.set_config(**kw) if kw else c.get_config()
    print(cfg)
    return 0


def cmd_gui(c: ESP32Client, args) -> int:
    from .gui import run
    run(host=args.host)
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="esp32gif",
        description="Gerencia GIFs e a tela do GIF Display (ESP32) pela rede.")
    p.add_argument("--host", default=DEFAULT_HOST,
                   help=f"IP ou hostname do ESP32 (padrao: {DEFAULT_HOST})")
    sub = p.add_subparsers(dest="command", required=True)

    sub.add_parser("info", help="mostra estado da tela e da flash").set_defaults(func=cmd_info)
    sub.add_parser("ls", help="lista os GIFs").set_defaults(func=cmd_ls)

    up = sub.add_parser("upload", help="envia GIF(s), redimensionando por padrao")
    up.add_argument("files", nargs="+", help="arquivos .gif")
    up.add_argument("--mode", choices=["stretch", "fit", "fill"], default="stretch",
                    help="como ajustar a tela (padrao: stretch)")
    up.add_argument("--width", type=int, default=DEFAULT_SIZE[0])
    up.add_argument("--height", type=int, default=DEFAULT_SIZE[1])
    up.add_argument("--no-resize", action="store_true",
                    help="envia sem redimensionar")
    up.set_defaults(func=cmd_upload)

    rm = sub.add_parser("rm", help="apaga um GIF pelo nome")
    rm.add_argument("name")
    rm.set_defaults(func=cmd_rm)

    pl = sub.add_parser("play", help="toca um GIF pelo indice")
    pl.add_argument("index", type=int)
    pl.set_defaults(func=cmd_play)

    sub.add_parser("next", help="proximo GIF").set_defaults(func=cmd_next)
    sub.add_parser("prev", help="GIF anterior").set_defaults(func=cmd_prev)

    cf = sub.add_parser("config", help="le/ajusta a configuracao da tela")
    cf.add_argument("--brightness", type=int, help="0-255")
    cf.add_argument("--rotation", type=int, choices=[0, 1, 2, 3])
    mode = cf.add_mutually_exclusive_group()
    mode.add_argument("--lock", action="store_true",
                      help="trava no GIF atual (nao troca sozinho)")
    mode.add_argument("--every", type=int, metavar="SEG",
                      help="troca de GIF a cada SEG segundos")
    mode.add_argument("--auto-advance", type=int, metavar="MS",
                      help="igual a --every, mas em ms (0 desliga)")
    cf.add_argument("--loop", type=lambda v: v.lower() in ("1", "true", "sim", "yes"),
                    help="true/false")
    cf.set_defaults(func=cmd_config)

    gui = sub.add_parser("gui", help="abre a janela desktop")
    gui.set_defaults(func=cmd_gui)

    return p


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    if args.command == "gui":
        return args.func(None, args)
    client = ESP32Client(args.host)
    try:
        rc = args.func(client, args)
    except requests.RequestException as e:
        print(f"erro de rede: {e}", file=sys.stderr)
        return 1
    if rc == 0:
        set_saved_host(args.host)  # lembra o ultimo host que funcionou
    return rc


if __name__ == "__main__":
    raise SystemExit(main())

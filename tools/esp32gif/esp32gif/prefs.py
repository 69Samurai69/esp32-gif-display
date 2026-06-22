"""Preferencias locais do app (lembra o ultimo host usado)."""
from __future__ import annotations

import json
import os
from pathlib import Path

# Pode ser sobrescrito por env var (util em testes).
PREFS_PATH = Path(os.environ.get("ESP32GIF_PREFS",
                                 str(Path.home() / ".esp32gif.json")))

FALLBACK_HOST = "gifdisplay.local"


def load_prefs() -> dict:
    try:
        return json.loads(PREFS_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def save_prefs(data: dict) -> None:
    try:
        PREFS_PATH.write_text(json.dumps(data, indent=2), encoding="utf-8")
    except OSError:
        pass


def get_saved_host() -> str | None:
    host = load_prefs().get("host")
    return host or None


def set_saved_host(host: str) -> None:
    host = (host or "").strip()
    if not host:
        return
    data = load_prefs()
    data["host"] = host
    save_prefs(data)


def default_host() -> str:
    """Host padrao: env var > ultimo salvo > fallback."""
    return os.environ.get("ESP32GIF_HOST") or get_saved_host() or FALLBACK_HOST

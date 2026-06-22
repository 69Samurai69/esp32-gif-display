"""Cliente HTTP para a API do GIF Display (ESP32)."""
from __future__ import annotations

import requests


class ESP32Client:
    """Fala com os endpoints /api/* expostos pelo firmware."""

    def __init__(self, host: str, timeout: float = 10.0):
        host = host.strip()
        if not host.startswith("http://") and not host.startswith("https://"):
            host = "http://" + host
        self.base = host.rstrip("/")
        self.timeout = timeout

    def _url(self, path: str) -> str:
        return self.base + path

    # --- GIFs ---------------------------------------------------------------
    def list_gifs(self) -> dict:
        r = requests.get(self._url("/api/gifs"), timeout=self.timeout)
        r.raise_for_status()
        return r.json()

    def upload(self, filename: str, data: bytes) -> None:
        files = {"gif": (filename, data, "image/gif")}
        # upload pode demorar: timeout maior
        r = requests.post(self._url("/api/upload"), files=files,
                          timeout=self.timeout * 6)
        r.raise_for_status()

    def delete(self, name: str) -> None:
        r = requests.post(self._url("/api/delete"), params={"name": name},
                          timeout=self.timeout)
        r.raise_for_status()

    # --- Reproducao ---------------------------------------------------------
    def play(self, index: int) -> None:
        r = requests.post(self._url("/api/play"), params={"index": index},
                          timeout=self.timeout)
        r.raise_for_status()

    def next(self) -> None:
        requests.post(self._url("/api/next"), timeout=self.timeout).raise_for_status()

    def prev(self) -> None:
        requests.post(self._url("/api/prev"), timeout=self.timeout).raise_for_status()

    # --- Configuracao da tela ----------------------------------------------
    def get_config(self) -> dict:
        r = requests.get(self._url("/api/config"), timeout=self.timeout)
        r.raise_for_status()
        return r.json()

    def set_config(self, **kwargs) -> dict:
        """Aceita: brightness, rotation, autoAdvanceMs, loop."""
        params = {k: int(v) for k, v in kwargs.items() if v is not None}
        r = requests.post(self._url("/api/config"), params=params,
                          timeout=self.timeout)
        r.raise_for_status()
        return r.json()

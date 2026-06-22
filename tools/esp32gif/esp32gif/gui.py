"""Janela desktop (Tkinter) para gerenciar o GIF Display."""
from __future__ import annotations

import os
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import requests

from .client import ESP32Client
from .prefs import default_host, set_saved_host
from .resize import DEFAULT_SIZE, resize_gif


def _human(n: int) -> str:
    return f"{n/1048576:.1f} MB" if n >= 1048576 else f"{n/1024:.0f} KB"


class App(tk.Tk):
    def __init__(self, host: str):
        super().__init__()
        self.title("GIF Display — ESP32")
        self.geometry("520x620")
        self.client: ESP32Client | None = None

        self._build_connect(host)
        self._build_playlist()
        self._build_upload()
        self._build_config()
        self._set_status("Desconecte/conecte para começar.")

        # Ao fechar, lembra o host digitado para a proxima vez.
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _on_close(self):
        set_saved_host(self.host_var.get())
        self.destroy()

    # --- construcao da UI ---------------------------------------------------
    def _build_connect(self, host: str):
        f = ttk.LabelFrame(self, text="Conexão")
        f.pack(fill="x", padx=8, pady=6)
        self.host_var = tk.StringVar(value=host)
        ttk.Entry(f, textvariable=self.host_var).pack(side="left", fill="x",
                                                      expand=True, padx=6, pady=6)
        ttk.Button(f, text="Conectar", command=self.connect).pack(side="left",
                                                                  padx=6)
        self.usage = ttk.Label(f, text="")
        self.usage.pack(side="left", padx=6)

    def _build_playlist(self):
        f = ttk.LabelFrame(self, text="GIFs no dispositivo")
        f.pack(fill="both", expand=True, padx=8, pady=6)
        self.listbox = tk.Listbox(f, height=8)
        self.listbox.pack(fill="both", expand=True, side="left", padx=6, pady=6)
        self.listbox.bind("<Double-Button-1>", lambda e: self.play_selected())
        bar = ttk.Frame(f)
        bar.pack(side="left", fill="y", padx=4)
        ttk.Button(bar, text="▶ Tocar", command=self.play_selected).pack(fill="x", pady=2)
        ttk.Button(bar, text="🗑 Apagar", command=self.delete_selected).pack(fill="x", pady=2)
        ttk.Separator(bar).pack(fill="x", pady=6)
        ttk.Button(bar, text="⟵ Anterior", command=lambda: self._do(lambda: self.client.prev())).pack(fill="x", pady=2)
        ttk.Button(bar, text="Próximo ⟶", command=lambda: self._do(lambda: self.client.next())).pack(fill="x", pady=2)
        ttk.Button(bar, text="↻ Atualizar", command=self.refresh).pack(fill="x", pady=2)

    def _build_upload(self):
        f = ttk.LabelFrame(self, text="Enviar GIF")
        f.pack(fill="x", padx=8, pady=6)
        self.mode_var = tk.StringVar(value="stretch")
        ttk.Label(f, text="Ajuste:").pack(side="left", padx=6)
        ttk.Combobox(f, textvariable=self.mode_var, width=9, state="readonly",
                     values=["stretch", "fit", "fill"]).pack(side="left")
        self.resize_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(f, text="redimensionar", variable=self.resize_var).pack(side="left", padx=6)
        ttk.Button(f, text="Escolher e enviar…", command=self.upload).pack(side="right", padx=6, pady=6)

    def _build_config(self):
        f = ttk.LabelFrame(self, text="Configuração da tela")
        f.pack(fill="x", padx=8, pady=6)

        ttk.Label(f, text="Brilho").grid(row=0, column=0, sticky="w", padx=6, pady=4)
        self.bright_var = tk.IntVar(value=160)
        ttk.Scale(f, from_=0, to=255, variable=self.bright_var,
                  orient="horizontal", length=200).grid(row=0, column=1, padx=6)

        ttk.Label(f, text="Rotação").grid(row=1, column=0, sticky="w", padx=6, pady=4)
        self.rot_var = tk.IntVar(value=1)
        ttk.Combobox(f, width=22, state="readonly", textvariable=self.rot_var,
                     values=[0, 1, 2, 3]).grid(row=1, column=1, sticky="w", padx=6)

        ttk.Label(f, text="Modo").grid(row=2, column=0, sticky="nw", padx=6, pady=4)
        self.mode_play_var = tk.StringVar(value="lock")
        mode_box = ttk.Frame(f)
        mode_box.grid(row=2, column=1, sticky="w", padx=6)
        ttk.Radiobutton(mode_box, text="Travar no GIF atual",
                        variable=self.mode_play_var, value="lock",
                        command=self._sync_mode).pack(anchor="w")
        line = ttk.Frame(mode_box)
        line.pack(anchor="w")
        ttk.Radiobutton(line, text="Trocar a cada", variable=self.mode_play_var,
                        value="cycle", command=self._sync_mode).pack(side="left")
        self.interval_var = tk.IntVar(value=10)
        self.interval_spin = ttk.Spinbox(line, from_=1, to=86400, width=6,
                                         textvariable=self.interval_var)
        self.interval_spin.pack(side="left", padx=4)
        ttk.Label(line, text="s").pack(side="left")

        ttk.Button(f, text="Aplicar configuração", command=self.apply_config).grid(
            row=3, column=1, sticky="e", padx=6, pady=6)
        self._sync_mode()

        self.status = ttk.Label(self, text="", anchor="w")
        self.status.pack(fill="x", side="bottom", padx=8, pady=4)

    def _sync_mode(self):
        # habilita o intervalo so no modo "trocar periodicamente"
        state = "normal" if self.mode_play_var.get() == "cycle" else "disabled"
        self.interval_spin.config(state=state)

    # --- helpers ------------------------------------------------------------
    def _set_status(self, msg: str):
        self.status.config(text=msg)
        self.update_idletasks()

    def _need_client(self) -> bool:
        if self.client is None:
            messagebox.showwarning("Sem conexão", "Conecte ao ESP32 primeiro.")
            return False
        return True

    def _do(self, fn):
        """Executa uma chamada simples tratando erros de rede."""
        if not self._need_client():
            return
        try:
            fn()
        except requests.RequestException as e:
            messagebox.showerror("Erro de rede", str(e))

    # --- acoes --------------------------------------------------------------
    def connect(self):
        host = self.host_var.get().strip()
        self.client = ESP32Client(host)
        try:
            cfg = self.client.get_config()
        except requests.RequestException as e:
            self.client = None
            messagebox.showerror("Falha ao conectar", str(e))
            return
        set_saved_host(host)  # conexao OK: lembra esse host
        self.bright_var.set(int(cfg.get("brightness", 160)))
        self.rot_var.set(int(cfg.get("rotation", 1)))
        auto = int(cfg.get("autoAdvanceMs", 0))
        if auto > 0:
            self.mode_play_var.set("cycle")
            self.interval_var.set(max(1, round(auto / 1000)))
        else:
            self.mode_play_var.set("lock")
        self._sync_mode()
        self._set_status(f"Conectado • tela {cfg['screenW']}x{cfg['screenH']}")
        self.refresh()

    def refresh(self):
        if not self._need_client():
            return
        try:
            d = self.client.list_gifs()
        except requests.RequestException as e:
            messagebox.showerror("Erro de rede", str(e))
            return
        self.listbox.delete(0, "end")
        cur = d.get("current", -1)
        for i, g in enumerate(d["gifs"]):
            mark = "▶ " if i == cur else "   "
            self.listbox.insert("end", f"{mark}{g['name']}  ({_human(g['size'])})")
        if 0 <= cur < self.listbox.size():
            self.listbox.selection_clear(0, "end")
            self.listbox.selection_set(cur)
        self.usage.config(text=f"{_human(d['used'])} / {_human(d['total'])}")

    def _selected_index(self) -> int | None:
        sel = self.listbox.curselection()
        return sel[0] if sel else None

    def _selected_name(self) -> str | None:
        i = self._selected_index()
        if i is None:
            return None
        # texto = "▶ nome  (123 KB)" -> extrai o nome
        text = self.listbox.get(i).strip()
        if text.startswith("▶"):
            text = text[1:].strip()
        return text.rsplit("  (", 1)[0]

    def play_selected(self):
        i = self._selected_index()
        if i is None:
            return
        self._do(lambda: self.client.play(i))
        self.after(300, self.refresh)

    def delete_selected(self):
        name = self._selected_name()
        if not name:
            return
        if not messagebox.askyesno("Apagar", f"Apagar {name}?"):
            return
        self._do(lambda: self.client.delete(name))
        self.after(200, self.refresh)

    def upload(self):
        if not self._need_client():
            return
        paths = filedialog.askopenfilenames(
            title="Escolha GIF(s)", filetypes=[("GIF", "*.gif"), ("Todos", "*.*")])
        if not paths:
            return
        threading.Thread(target=self._upload_worker, args=(list(paths),),
                         daemon=True).start()

    def _upload_worker(self, paths):
        mode = self.mode_var.get()
        do_resize = self.resize_var.get()
        for path in paths:
            name = os.path.basename(path)
            if not name.lower().endswith(".gif"):
                name += ".gif"
            try:
                self._set_status(f"Processando {name}…")
                if do_resize:
                    data = resize_gif(path, size=DEFAULT_SIZE, mode=mode)
                else:
                    with open(path, "rb") as fh:
                        data = fh.read()
                self._set_status(f"Enviando {name} ({_human(len(data))})…")
                self.client.upload(name, data)
            except (requests.RequestException, OSError, ValueError) as e:
                messagebox.showerror("Erro no upload", f"{name}: {e}")
                continue
        self._set_status("Upload concluído.")
        self.refresh()

    def apply_config(self):
        if not self._need_client():
            return
        cycle = self.mode_play_var.get() == "cycle"
        auto = max(1, self.interval_var.get()) * 1000 if cycle else 0
        try:
            cfg = self.client.set_config(
                brightness=self.bright_var.get(),
                rotation=self.rot_var.get(),
                autoAdvanceMs=auto,
                loop=1,  # sempre loop: trava o GIF ou repete cada um ate trocar
            )
            modo = (f"trocando a cada {self.interval_var.get()}s" if cycle
                    else "travado no GIF atual")
            self._set_status(f"Configuração aplicada • {modo}")
        except requests.RequestException as e:
            messagebox.showerror("Erro de rede", str(e))


def run(host: str | None = None):
    App(host or default_host()).mainloop()


if __name__ == "__main__":
    run()

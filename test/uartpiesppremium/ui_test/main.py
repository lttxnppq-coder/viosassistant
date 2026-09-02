"""Smart AC Control — UI Test (dashboard Tkinter, isolated prototype).

UI doc lap, chi fake state + keyboard simulation:

    Keyboard
       ↓
    Fake UI State (ui_state.py)
       ↓
    Tkinter Dashboard

KHONG ket noi ESP32, Vosk, UART, microphone, audio hay bat ky module
nao cua project chinh. Chay:

    python ui_test/main.py

Verify tu dong (khong popup lau, tu dong dong cua so):

    python ui_test/main.py --smoke
"""

import argparse
import sys
import tkinter as tk
from tkinter import font as tkfont

from ui_state import UIState

# ---------------------------------------------------------------------------
# THEME — dark automotive
# ---------------------------------------------------------------------------

BG = "#0f1419"
CARD = "#1b232b"
TEXT = "#e8edf2"
DIM = "#7a8694"
ON_COLOR = "#34d399"
OFF_COLOR = "#5b6673"

STATUS_COLORS = {
    "READY": "#34d399",
    "LISTENING": "#f5b041",
    "PROCESSING": "#5dade2",
    "EXECUTING": "#e67e22",
}

FONT = "Segoe UI"

TEMP_KEYS = {
    "Z": 18, "X": 19, "C": 20, "V": 21, "B": 22,
    "3": 23, "4": 24, "5": 25, "6": 26, "7": 27, "8": 28, "9": 29, "0": 30,
}


class SmartACApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.state = UIState()
        self._pending_status: list[int] = []

        root.title("Smart AC Control — UI Test")
        root.geometry("900x600")
        root.minsize(760, 520)
        root.configure(bg=BG)

        self._build_widgets()
        self._bind_keys()
        self._refresh()

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_widgets(self) -> None:
        root = self.root
        root.grid_rowconfigure(1, weight=1)
        root.grid_columnconfigure(0, weight=1)

        self.var_system = tk.StringVar()
        self.var_ac = tk.StringVar()
        self.var_temp = tk.StringVar()
        self.var_fan = tk.StringVar()
        self.var_wind = tk.StringVar()
        self.var_mode = tk.StringVar()
        self.var_defrost = tk.StringVar()
        self.var_last = tk.StringVar()

        pad = {"padx": 18, "pady": 10}

        # ---- Header ----
        header = tk.Frame(root, bg=BG)
        header.grid(row=0, column=0, sticky="ew", **pad)
        header.grid_columnconfigure(1, weight=1)

        tk.Label(header, text="SMART AC", font=(FONT, 18, "bold"),
                 fg=TEXT, bg=BG).grid(row=0, column=0, sticky="w")

        self.dot_header = tk.Label(header, text="●", font=(FONT, 16), bg=BG)
        self.dot_header.grid(row=0, column=1, sticky="e")
        self.lbl_system_header = tk.Label(header, textvariable=self.var_system,
                                          font=(FONT, 13, "bold"), bg=BG)
        self.lbl_system_header.grid(row=0, column=2, sticky="w", padx=(6, 0))

        # ---- Main card: temperature ----
        main_card = tk.Frame(root, bg=CARD, highlightbackground="#26313b",
                             highlightthickness=1)
        main_card.grid(row=1, column=0, sticky="nsew", **pad)
        main_card.grid_rowconfigure(0, weight=1)
        main_card.grid_columnconfigure(0, weight=1)

        self.lbl_temp = tk.Label(main_card, textvariable=self.var_temp,
                                 font=(FONT, 64, "bold"), fg=TEXT, bg=CARD)
        self.lbl_temp.grid(row=0, column=0, pady=(18, 0))

        tk.Label(main_card, text="AIR CONDITIONER", font=(FONT, 13),
                 fg=DIM, bg=CARD).grid(row=1, column=0)

        self.lbl_ac = tk.Label(main_card, textvariable=self.var_ac,
                               font=(FONT, 22, "bold"), bg=CARD)
        self.lbl_ac.grid(row=2, column=0, pady=(4, 18))

        # ---- Mini cards: FAN / WIND / MODE / DEFROST ----
        mini_row = tk.Frame(root, bg=BG)
        mini_row.grid(row=2, column=0, sticky="ew", **pad)
        for c in range(4):
            mini_row.grid_columnconfigure(c, weight=1)

        self.lbl_fan = self._make_mini_card(mini_row, 0, "FAN", self.var_fan)
        self.lbl_wind = self._make_mini_card(mini_row, 1, "WIND", self.var_wind)
        self.lbl_mode = self._make_mini_card(mini_row, 2, "MODE", self.var_mode)
        self.lbl_defrost = self._make_mini_card(mini_row, 3, "DEFROST", self.var_defrost)

        # ---- Last command ----
        last_card = tk.Frame(root, bg=CARD, highlightbackground="#26313b",
                             highlightthickness=1)
        last_card.grid(row=3, column=0, sticky="ew", **pad)
        last_card.grid_columnconfigure(0, weight=1)

        tk.Label(last_card, text="LAST COMMAND", font=(FONT, 10, "bold"),
                 fg=DIM, bg=CARD).grid(row=0, column=0, sticky="w", padx=14, pady=(10, 2))
        self.lbl_last = tk.Label(last_card, textvariable=self.var_last,
                                 font=(FONT, 14), fg=TEXT, bg=CARD)
        self.lbl_last.grid(row=1, column=0, sticky="w", padx=14, pady=(0, 12))

        # ---- Footer ----
        footer = tk.Frame(root, bg=BG)
        footer.grid(row=4, column=0, sticky="ew", **pad)

        self.dot_footer = tk.Label(footer, text="●", font=(FONT, 14), bg=BG)
        self.dot_footer.pack(side="left")
        self.lbl_system_footer = tk.Label(footer, textvariable=self.var_system,
                                          font=(FONT, 12, "bold"), bg=BG)
        self.lbl_system_footer.pack(side="left", padx=(6, 0))
        tk.Label(footer, text="Keyboard simulation mode", font=(FONT, 10),
                 fg=DIM, bg=BG).pack(side="right")

    def _make_mini_card(self, parent, col, title, var) -> tk.Label:
        card = tk.Frame(parent, bg=CARD, highlightbackground="#26313b",
                        highlightthickness=1)
        card.grid(row=0, column=col, sticky="ew", padx=(0, 8))
        card.grid_columnconfigure(0, weight=1)

        tk.Label(card, text=title, font=(FONT, 10, "bold"), fg=DIM,
                 bg=CARD).grid(row=0, column=0, pady=(12, 2))
        lbl = tk.Label(card, textvariable=var, font=(FONT, 18, "bold"),
                       fg=TEXT, bg=CARD)
        lbl.grid(row=1, column=0, pady=(0, 12))
        return lbl

    # ------------------------------------------------------------------
    # Keyboard simulation
    # ------------------------------------------------------------------

    def _bind_keys(self) -> None:
        self.root.bind("<KeyPress>", self._on_key)

    def _on_key(self, event: tk.Event) -> None:
        if event.char:
            key = event.char.upper()
        else:
            key = event.keysym  # "Up" / "Down"
        self.handle_key(key)

    def handle_key(self, key: str) -> None:
        """Xu ly phim gia lap; key la '1'..'9','0','Z','X','C','V','B','Up','Down','F','G','M','K','D','U','R'."""
        state = self.state

        if key == "1":
            state.set_ac(True)
            state.last_command = "Đã bật điều hòa"
        elif key == "2":
            state.set_ac(False)
            state.last_command = "Đã tắt điều hòa"
        elif key == "Up":
            state.temp_up()
            state.last_command = "Đã tăng nhiệt độ lên hai độ"
        elif key == "Down":
            state.temp_down()
            state.last_command = "Đã giảm nhiệt độ xuống hai độ"
        elif key == "F":
            state.set_fan(True)
            state.last_command = "Đã bật quạt"
        elif key == "G":
            state.set_fan(False)
            state.last_command = "Đã tắt quạt"
        elif key == "M":
            state.set_wind(True)
            state.last_command = "Đã chuyển hướng gió lên mặt"
        elif key == "K":
            state.set_wind(False)
            state.last_command = "Đã chuyển hướng gió xuống chân"
        elif key == "D":
            if state.toggle_defrost():
                state.last_command = "Đã bật chế độ sưởi kính chắn gió"
            else:
                state.last_command = "Đã tắt chế độ sưởi kính chắn gió"
        elif key in TEMP_KEYS:
            state.set_temperature(TEMP_KEYS[key])
            state.last_command = f"Đã đặt nhiệt độ {state.temperature} độ"
        elif key == "U":
            state.last_command = "Tôi chưa hiểu yêu cầu của bạn"
        elif key == "R":
            state.reset()
        else:
            return

        self._refresh()
        self._run_status_cycle()

    # ------------------------------------------------------------------
    # Status cycle + refresh
    # ------------------------------------------------------------------

    def _run_status_cycle(self) -> None:
        for after_id in self._pending_status:
            self.root.after_cancel(after_id)
        self._pending_status = []

        for delay, status in ((0, "LISTENING"), (120, "PROCESSING"),
                              (240, "EXECUTING"), (380, "READY")):
            after_id = self.root.after(delay, lambda s=status: self._set_status(s))
            self._pending_status.append(after_id)

    def _set_status(self, status: str) -> None:
        self.state.system = status
        color = STATUS_COLORS.get(status, ON_COLOR)
        self.var_system.set(status)
        self.dot_header.configure(fg=color)
        self.dot_footer.configure(fg=color)
        self.lbl_system_header.configure(fg=color)
        self.lbl_system_footer.configure(fg=color)

    def _refresh(self) -> None:
        s = self.state
        self._set_status(s.system)

        self.var_temp.set(f"{s.temperature}°C")
        self.var_ac.set("ON" if s.ac_on else "OFF")
        self.lbl_ac.configure(fg=ON_COLOR if s.ac_on else OFF_COLOR)

        self.var_fan.set("ON" if s.fan else "OFF")
        self.lbl_fan.configure(fg=ON_COLOR if s.fan else OFF_COLOR)

        self.var_wind.set(s.wind)
        self.var_mode.set(s.mode)
        self.var_defrost.set("ON" if s.defrost else "OFF")
        self.lbl_defrost.configure(fg=ON_COLOR if s.defrost else OFF_COLOR)

        self.var_last.set(f"✓ {s.last_command}")


# ---------------------------------------------------------------------------
# Smoke test — tu dong gui phim, verify state, dong cua so
# ---------------------------------------------------------------------------

SMOKE_STEPS = [
    ("1", lambda s: s.ac_on is True, "AC ON"),
    ("2", lambda s: s.ac_on is False, "AC OFF"),
    ("Up", lambda s: s.temperature == 27, "TEMP +2 (25 -> 27)"),
    ("Down", lambda s: s.temperature == 25, "TEMP -2 (27 -> 25)"),
    ("F", lambda s: s.fan is True, "FAN ON"),
    ("G", lambda s: s.fan is False, "FAN OFF"),
    ("M", lambda s: s.wind == "FACE", "WIND FACE"),
    ("K", lambda s: s.wind == "FOOT", "WIND FOOT"),
    ("D", lambda s: s.defrost is True, "DEFROST ON"),
    ("5", lambda s: s.temperature == 25, "SET TEMP 25"),
    ("Z", lambda s: s.temperature == 18, "SET TEMP 18"),
    ("Down", lambda s: s.temperature == 18, "BOUNDARY LOW (18 - 2 -> 18)"),
    ("X", lambda s: s.temperature == 19, "SET TEMP 19"),
    ("C", lambda s: s.temperature == 20, "SET TEMP 20"),
    ("V", lambda s: s.temperature == 21, "SET TEMP 21"),
    ("B", lambda s: s.temperature == 22, "SET TEMP 22"),
    ("3", lambda s: s.temperature == 23, "SET TEMP 23"),
    ("4", lambda s: s.temperature == 24, "SET TEMP 24"),
    ("6", lambda s: s.temperature == 26, "SET TEMP 26"),
    ("7", lambda s: s.temperature == 27, "SET TEMP 27"),
    ("8", lambda s: s.temperature == 28, "SET TEMP 28"),
    ("9", lambda s: s.temperature == 29, "SET TEMP 29"),
    ("0", lambda s: s.temperature == 30, "SET TEMP 30"),
    ("Up", lambda s: s.temperature == 30, "BOUNDARY HIGH (30 + 2 -> 30)"),
    ("U", lambda s: s.last_command == "Tôi chưa hiểu yêu cầu của bạn", "UNKNOWN"),
    ("R", lambda s: (s.ac_on is False and s.temperature == 25 and s.fan is False
                     and s.defrost is False and s.last_command == "Ready"), "RESET"),
]


def run_smoke(root: tk.Tk, app: SmartACApp) -> int:
    failed = 0
    for key, pred, label in SMOKE_STEPS:
        app.handle_key(key)
        ok = pred(app.state)
        if not ok:
            failed += 1
        print(f"[{'PASS' if ok else 'FAIL'}] {label} (key={key})")
    print(f"SMOKE: {len(SMOKE_STEPS) - failed}/{len(SMOKE_STEPS)} passed")

    root.after(150, root.destroy)
    root.mainloop()
    return 1 if failed else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Smart AC Control — UI Test")
    parser.add_argument("--smoke", action="store_true",
                        help="Tu dong gui phim test + boundary, dong cua so, exit 0/1")
    args = parser.parse_args()

    root = tk.Tk()
    app = SmartACApp(root)

    if args.smoke:
        return run_smoke(root, app)
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
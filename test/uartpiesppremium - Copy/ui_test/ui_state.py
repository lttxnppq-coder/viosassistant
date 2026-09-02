"""Fake UI state cho Smart AC dashboard - hoan toan doc lap (chi stdlib).

KHONG import bat ky module nao cua project chinh (command_ai, vosk_test,
esp32_sender, serial, sounddevice, vosk, ...). Day la UI prototype/test
isolated: khong ket noi ESP32, Vosk, UART, microphone hay audio.
"""

TEMP_MIN = 18
TEMP_MAX = 30
TEMP_STEP = 2


class UIState:
    """Giu trang thai UI gia lap. Khong over-engineering."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self.ac_on = False
        self.temperature = 25
        self.fan = False
        self.wind = "FACE"
        self.mode = "COOL"
        self.defrost = False
        self.system = "READY"
        self.last_command = "Ready"

    def set_ac(self, on: bool) -> None:
        self.ac_on = on

    def set_temperature(self, value: int) -> None:
        self.temperature = max(TEMP_MIN, min(TEMP_MAX, value))

    def temp_up(self) -> None:
        self.set_temperature(self.temperature + TEMP_STEP)

    def temp_down(self) -> None:
        self.set_temperature(self.temperature - TEMP_STEP)

    def set_fan(self, on: bool) -> None:
        self.fan = on

    def set_wind(self, face: bool) -> None:
        self.wind = "FACE" if face else "FOOT"

    def toggle_defrost(self) -> bool:
        self.defrost = not self.defrost
        return self.defrost
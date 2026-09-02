"""Command normalization cho pipeline Vosk -> ESP32.

Quy tac:
- Unicode NFD de tach dau, bo combining marks.
- Lowercase, gop khoang trang thua, bo dau cau.
- EXACT MATCH sau normalize -> command canonical.
- KHONG dung substring matching: "toi muon tien len" -> None (khong phai FORWARD).
"""

import re
import unicodedata

_COMMAND_MAP = {
    "xin chao": "HELLO",
    "hello": "HELLO",
    "tien len": "FORWARD",
    "lui lai": "BACKWARD",
    "re trai": "LEFT",
    "re phai": "RIGHT",
    "dung": "STOP",
    "dung lai": "STOP",
    "stop": "STOP",
}

_PUNCTUATION_RE = re.compile(r"[\.,!?;:]+")


def normalize_text(text: str) -> str:
    """Bo dau tieng Viet, lowercase, chuan hoa khoang trang va dau cau."""
    text = unicodedata.normalize("NFD", text)
    text = "".join(ch for ch in text if unicodedata.category(ch) != "Mn")
    text = text.lower()
    text = _PUNCTUATION_RE.sub(" ", text)
    text = " ".join(text.split())
    return text.strip()


def normalize_command(text: str | None) -> str | None:
    """Map text thanh command canonical (HELLO/FORWARD/...).

    Tra ve None neu khong phai command hop le (EXACT match).
    """
    if not text:
        return None
    key = normalize_text(text)
    return _COMMAND_MAP.get(key)
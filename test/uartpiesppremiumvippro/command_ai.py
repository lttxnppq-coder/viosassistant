"""command_ai.py - RULE-BASED command classifier (LOCAL, deterministic, khong AI/API).

HARDENED: chong FALSE POSITIVE tu text rac do Vosk sinh ra.
- Token/phrase matching co WORD BOUNDARY (khong substring "bat" trong "bat qua"...).
- AC/FAN BAT BUOC co context device (dieu hoa / may lanh / quat).
- NEGATION GATE: "khong/chua/dung(đừng)" -> UNKNOWN (khong doan lenh).
- Nhiet do rac: INVALID chi khi co dong tu set ("dat/chinh/cai/de nhiet do").
  Text rac chi tinh co chu "nhiet do" -> UNKNOWN.

Command code (da xac nhan):
    AC_ON=1  AC_OFF=2  TEMP_UP=4  TEMP_DOWN=5  FAN_ON=6  FAN_OFF=7
    AIR_FACE=8  AIR_FOOT=9  AIR_DEFROST=10
    SET_TEMPERATURE = 300 + nhiet do  (23->323 ... 30->330)
    HELLO / UNKNOWN / INVALID_TEMPERATURE = None

Chay test:  python test_command_ai.py
"""

import re
import unicodedata
from dataclasses import dataclass, field

# =========================================================
# COMMAND CODES
# =========================================================

COMMAND_CODES = {
    "AC_ON": 1,
    "AC_OFF": 2,
    "TEMP_UP": 4,
    "TEMP_DOWN": 5,
    "FAN_ON": 6,
    "FAN_OFF": 7,
    "AIR_FACE": 8,
    "AIR_FOOT": 9,
    "AIR_DEFROST": 10,
}

TEMP_MIN = 23
TEMP_MAX = 30
TEMP_BASE = 300  # SET_TEMPERATURE = 300 + nhiet do

SET_CONFIDENCE = 0.95
INVALID_CONFIDENCE = 0.85
UNKNOWN_CONFIDENCE = 0.0

MIN_SCORE = 2  # keyword scoring chi chon khi score >= 2

# =========================================================
# AIResult
# =========================================================


@dataclass
class AIResult:
    intent: str
    command_code: int | None
    temperature: int | None
    confidence: float
    raw_text: str
    normalized_text: str
    matched_keywords: list[str] = field(default_factory=list)
    response: str = ""


# =========================================================
# NORMALIZE
# =========================================================

_PUNCTUATION_RE = re.compile(r"[\.,!?;:\"']+")


def normalize_text(text: str) -> str:
    """Lowercase, NFD bo dau tieng Viet (ca 'đ' -> 'd'), gop khoang trang, bo dau cau.

    KHONG sua raw_text (luu rieng de debug).
    """
    if not text:
        return ""
    text = unicodedata.normalize("NFD", text)
    text = "".join(ch for ch in text if unicodedata.category(ch) != "Mn")
    text = text.lower()
    text = text.replace("đ", "d")  # U+0111 khong bi NFD phan ra -> xu ly rieng
    text = _PUNCTUATION_RE.sub(" ", text)
    text = " ".join(text.split())
    return text.strip()


# =========================================================
# TOKEN / PHRASE MATCHING (WORD BOUNDARY - KHONG substring)
# =========================================================


def _tokenize(normalized: str) -> list[str]:
    return normalized.split()


def _phrase_matches(normalized: str, phrase: str) -> bool:
    """Phrase chi match khi la token/cum tu doc lap.

    "bat" match trong "bat dieu hoa" nhung KHONG match trong "bat qua"
    (vi "qua" bat dau bang [a-z]).
    """
    if not phrase:
        return False
    return (
        re.search(r"(?<![a-z0-9])" + re.escape(phrase) + r"(?![a-z0-9])", normalized)
        is not None
    )


def _score_keywords(normalized: str, keywords: dict[str, int]) -> tuple[int, list[str]]:
    """Score bang phrase matching co word boundary. Tra (score, danh sach khop)."""
    score = 0
    matched: list[str] = []
    for kw, w in keywords.items():
        if _phrase_matches(normalized, kw):
            score += w
            matched.append(kw)
    return score, matched


# =========================================================
# NHIET DO: chu so + so chu tieng Viet (0-99)
# =========================================================

_UNITS = {
    "khong": 0, "mot": 1, "hai": 2, "ba": 3, "bon": 4,
    "nam": 5, "sau": 6, "bay": 7, "tam": 8, "chin": 9,
}

_UNIT_AFTER_MUOI = {
    "lam": 5, "nam": 5, "tu": 4, "khong": 0,
    "mot": 1, "hai": 2, "ba": 3, "bon": 4,
    "sau": 6, "bay": 7, "tam": 8, "chin": 9,
}

_TENS = {
    "hai": 2, "ba": 3, "bon": 4, "nam": 5,
    "sau": 6, "bay": 7, "tam": 8, "chin": 9,
}

_DIGITS_RE = re.compile(r"(?<!\d)\d{1,2}(?!\d)")

_WORD_NUM_RE = re.compile(
    r"(?<![a-z])(?:"
    r"(?:hai|ba|bon|nam|sau|bay|tam|chin)\s+muoi"
    r"(?:\s+(?:lam|nam|tu|mot|hai|ba|bon|sau|bay|tam|chin|khong))?"
    r"|muoi(?:\s+(?:lam|nam|tu|mot|hai|ba|bon|sau|bay|tam|chin|khong))?"
    r"|khong|mot|hai|ba|bon|nam|sau|bay|tam|chin"
    r")(?![a-z])"
)


def _word_number_value(match: re.Match) -> int:
    s = match.group(0)
    if "muoi" in s:
        parts = s.split()
        if parts[0] == "muoi":
            tens = 10
            unit_word = parts[1] if len(parts) > 1 else None
        else:
            tens = _TENS[parts[0]] * 10
            unit_word = parts[2] if len(parts) > 2 else None
        if unit_word:
            return tens + _UNIT_AFTER_MUOI[unit_word]
        return tens
    return _UNITS[s]


def _extract_numbers(text: str) -> list[int]:
    """Trich tat ca so (chu so + so chu tieng Viet) trong text, 0-99."""
    nums: list[int] = []
    for m in _DIGITS_RE.finditer(text):
        nums.append(int(m.group(0)))
    for m in _WORD_NUM_RE.finditer(text):
        nums.append(_word_number_value(m))
    return nums


# =========================================================
# NEGATION GATE
# =========================================================

_NEGATION_TOKENS = {"khong", "chua", "dung"}  # "dung" = đừng (an toan -> UNKNOWN)

# =========================================================
# AC / FAN: CONTEXT REQUIREMENT (dong tu + device)
# =========================================================

_AC_DEVICE_PHRASES = ("dieu hoa", "may lanh", "may dieu hoa")
_FAN_DEVICE_PHRASES = ("quat",)

_AC_ON_VERBS = ("mo", "bat", "chay")
_AC_OFF_VERBS = ("tat", "dong", "ngung", "ngat")
_FAN_ON_VERBS = ("mo", "bat")
_FAN_OFF_VERBS = ("tat", "ngung", "ngat")

# =========================================================
# KEYWORD SCORING (TEMP_UP/DOWN + AIR + HELLO)
# =========================================================

_INTENT_KEYWORDS: dict[str, dict[str, int]] = {
    "HELLO": {
        "xin chao": 2, "hello": 2, "chao ban": 2, "chao": 1,
    },
    "TEMP_UP": {
        "tang": 2, "tang nhiet": 2, "tang len": 2,
        "nhiet do": 1, "nong hon": 2, "nong len": 2,
        "am hon": 2, "am len": 2, "len": 1,
    },
    "TEMP_DOWN": {
        "giam": 2, "giam nhiet": 2, "giam xuong": 2,
        "nhiet do": 1, "lanh hon": 2, "lanh di": 2,
        "ha nhiet": 2, "bot nhiet": 2, "xuong": 1,
    },
    "AIR_FACE": {
        "huong gio": 2, "len mat": 2, "thoi vao mat": 2,
        "thoi len": 2, "gio": 1, "mat": 1, "len": 1,
    },
    "AIR_FOOT": {
        "huong gio": 2, "xuong chan": 2, "thoi xuong chan": 2,
        "thoi xuong": 2, "gio": 1, "chan": 1, "xuong": 1,
    },
    "AIR_DEFROST": {
        "kinh chan gio": 2, "suoi kinh": 2, "say kinh": 2,
        "thoi kinh": 2, "huong gio": 2, "kinh": 2,
        "gio": 1, "chan gio": 1,
    },
}

# INVALID_TEMPERATURE CHI khi co dong tu set + "nhiet do" ma KHONG co so hop le.
# "nhiet do" tran (text rac) KHONG du tieu chi -> UNKNOWN.
_SET_TEMP_PHRASES = (
    "dat nhiet do", "chinh nhiet do", "cai nhiet do", "de nhiet do",
    "dat nhiet", "chinh nhiet",
)

# =========================================================
# RESPONSES
# =========================================================

_RESPONSES = {
    "AC_ON": "Đã bật điều hòa.",
    "AC_OFF": "Đã tắt điều hòa.",
    "TEMP_UP": "Đã tăng nhiệt độ.",
    "TEMP_DOWN": "Đã giảm nhiệt độ.",
    "FAN_ON": "Đã bật quạt.",
    "FAN_OFF": "Đã tắt quạt.",
    "AIR_FACE": "Đã hướng gió lên mặt.",
    "AIR_FOOT": "Đã hướng gió xuống chân.",
    "AIR_DEFROST": "Đã hướng gió sưởi kính chắn gió.",
    "HELLO": "Xin chào! Tôi đang hoạt động.",
    "UNKNOWN": "Tôi chưa hiểu yêu cầu.",
    "INVALID_TEMPERATURE": "Nhiệt độ cho phép từ 23 đến 30 độ.",
}


# =========================================================
# CLASSIFY
# =========================================================

def classify(raw_text: str) -> AIResult:
    """Phan loai cau noi -> AIResult (deterministic, local, khong AI/API).

    Thu tu uu tien:
      NEGATION GATE -> NHIET DO CU THE -> TANG/GIAM -> SET khong co so
      -> AC/FAN (context) -> KEYWORD SCORING (AIR/HELLO) -> UNKNOWN
    """

    normalized = normalize_text(raw_text)
    tokens = _tokenize(normalized)
    word_count = max(1, len(tokens))

    def make(intent: str, code: int | None, temp: int | None,
             conf: float, matched: list[str]) -> AIResult:
        response = _RESPONSES.get(intent, _RESPONSES["UNKNOWN"])
        if intent == "SET_TEMPERATURE" and temp is not None:
            response = f"Đã đặt nhiệt độ {temp} độ."
        return AIResult(
            intent=intent,
            command_code=code,
            temperature=temp,
            confidence=conf,
            raw_text=raw_text,
            normalized_text=normalized,
            matched_keywords=matched,
            response=response,
        )

    if not normalized:
        return make("UNKNOWN", None, None, UNKNOWN_CONFIDENCE, [])

    # ---- B0: NEGATION GATE (an toan nhat: co phu dinh -> KHONG doan lenh) ----
    for t in tokens:
        if t in _NEGATION_TOKENS:
            return make(
                "UNKNOWN", None, None, UNKNOWN_CONFIDENCE, [f"phu-dinh={t}"]
            )

    # ---- B1: NHIET DO CU THE (uu tien nhat) ----
    # Context: "nhiet" co trong cau, HOAC "do" dung NGAY SAU mot so ("30 do", "hai muoi lam do").
    # Token "do" don le (vi du "do" = đó) KHONG duoc tinh la "do".
    numbers = _extract_numbers(normalized)
    has_temp_ctx = "nhiet" in normalized
    if not has_temp_ctx and numbers:
        for m in list(_DIGITS_RE.finditer(normalized)) + list(_WORD_NUM_RE.finditer(normalized)):
            rest = normalized[m.end():].lstrip()
            if rest == "do" or rest.startswith("do "):
                has_temp_ctx = True
                break
    if numbers and has_temp_ctx:
        for n in numbers:
            if TEMP_MIN <= n <= TEMP_MAX:
                return make(
                    "SET_TEMPERATURE", TEMP_BASE + n, n,
                    SET_CONFIDENCE, [f"so={n}", "context=nhiet-do"],
                )
        # Co so nhung ngoai 23-30 -> KHONG tu ep, KHONG doan
        return make(
            "INVALID_TEMPERATURE", None, None,
            INVALID_CONFIDENCE, [f"so={numbers[0]}", "ngoai-khoang-23-30"],
        )

    # ---- B2: TANG/GIAM NHIET DO (uu tien hon con so don le) ----
    up_score, up_kw = _score_keywords(normalized, _INTENT_KEYWORDS["TEMP_UP"])
    down_score, down_kw = _score_keywords(normalized, _INTENT_KEYWORDS["TEMP_DOWN"])
    if up_score >= MIN_SCORE and up_score >= down_score:
        return make("TEMP_UP", COMMAND_CODES["TEMP_UP"], None,
                    round(min(1.0, up_score / word_count), 2), up_kw)
    if down_score >= MIN_SCORE and down_score > up_score:
        return make("TEMP_DOWN", COMMAND_CODES["TEMP_DOWN"], None,
                    round(min(1.0, down_score / word_count), 2), down_kw)

    # ---- B3: y "dat/chinh nhiet do" nhung KHONG co so hop le ----
    if any(_phrase_matches(normalized, p) for p in _SET_TEMP_PHRASES):
        # Gate rac: "hoac"/"hay" trong cau set-temp khong co so = liet ke rac (Vosk) -> UNKNOWN
        if any(t in ("hoac", "hay") for t in tokens):
            return make("UNKNOWN", None, None, UNKNOWN_CONFIDENCE, ["rac=hoac/hay"])
        return make(
            "INVALID_TEMPERATURE", None, None,
            INVALID_CONFIDENCE, ["co-y-dat-nhiet-do", "khong-co-so-hop-le"],
        )

    # ---- B4: AC/FAN - BAT BUOC dong tu + device context ----
    ac_device = next((p for p in _AC_DEVICE_PHRASES
                      if _phrase_matches(normalized, p)), None)
    fan_device = next((p for p in _FAN_DEVICE_PHRASES
                       if _phrase_matches(normalized, p)), None)

    if ac_device:
        for v in _AC_ON_VERBS:
            if _phrase_matches(normalized, v):
                return make("AC_ON", COMMAND_CODES["AC_ON"], None,
                            round(min(1.0, 4 / word_count), 2), [v, ac_device])
        for v in _AC_OFF_VERBS:
            if _phrase_matches(normalized, v):
                return make("AC_OFF", COMMAND_CODES["AC_OFF"], None,
                            round(min(1.0, 4 / word_count), 2), [v, ac_device])

    if fan_device:
        for v in _FAN_ON_VERBS:
            if _phrase_matches(normalized, v):
                return make("FAN_ON", COMMAND_CODES["FAN_ON"], None,
                            round(min(1.0, 4 / word_count), 2), [v, fan_device])
        for v in _FAN_OFF_VERBS:
            if _phrase_matches(normalized, v):
                return make("FAN_OFF", COMMAND_CODES["FAN_OFF"], None,
                            round(min(1.0, 4 / word_count), 2), [v, fan_device])

    # ---- B5: KEYWORD SCORING (AIR / HELLO) ----
    best_intent: str | None = None
    best_score = 0
    tied = False
    best_kw: list[str] = []

    for intent, keywords in _INTENT_KEYWORDS.items():
        if intent in ("TEMP_UP", "TEMP_DOWN"):
            continue  # da xu ly o B2
        score, matched = _score_keywords(normalized, keywords)
        if score > best_score:
            best_score = score
            best_intent = intent
            best_kw = matched
            tied = False
        elif score == best_score and score > 0:
            tied = True

    # SAFETY: diem thap hoac nhieu intent cung diem -> UNKNOWN, khong doan
    if best_intent is None or best_score < MIN_SCORE or tied:
        return make("UNKNOWN", None, None, UNKNOWN_CONFIDENCE, [])

    return make(
        best_intent, COMMAND_CODES.get(best_intent), None,
        round(min(1.0, best_score / word_count), 2), best_kw,
    )


if __name__ == "__main__":
    # Debug nhanh: python command_ai.py "cau thu"
    import sys

    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    for arg in sys.argv[1:]:
        r = classify(arg)
        print(f"RAW: {r.raw_text}")
        print(f"NORMALIZED: {r.normalized_text}")
        print(f"INTENT: {r.intent}")
        print(f"CONFIDENCE: {r.confidence}")
        print(f"MATCHED: {r.matched_keywords}")
        print(f"COMMAND: {r.command_code}")
        print(f"RESPONSE: {r.response}")
        print()
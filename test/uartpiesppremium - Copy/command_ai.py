"""command_ai.py - RULE-BASED command classifier (LOCAL, deterministic, khong AI/API).

HARDENED: chong FALSE POSITIVE tu text rac do Vosk sinh ra.
- Token/phrase matching co WORD BOUNDARY (khong substring "bat" trong "bat qua"...).
- AC/FAN BAT BUOC co context device (dieu hoa / may lanh / quat).
- NEGATION GATE: "khong/chua/dung(đừng)" -> UNKNOWN (khong doan lenh).
- Nhiet do rac: INVALID chi khi co dong tu set ("dat/chinh/cai/de nhiet do").
  Text rac chi tinh co chu "nhiet do" -> UNKNOWN.

Chuan hoa da tang (xem command_normalizer.py):
    TANG 1 normalize_text() + TANG 3 correct_stt_errors() -> text match.
    TANG 2 canonicalize() chi dung cho hien thi debug (khong match).

Command code (da xac nhan):
    AC_ON=1  AC_OFF=2  TEMP_UP=4  TEMP_DOWN=5  FAN_ON=6  FAN_OFF=7
    AIR_FACE=8  AIR_FOOT=9  AIR_DEFROST=10  AIR_AUTO=11
    SET_TEMPERATURE = 300 + nhiet do  (18->318 ... 30->330)
    HELLO / GOODBYE / UNKNOWN / INVALID_TEMPERATURE = None (code khong gui ESP32)

Chay test:  python test_command_ai.py
"""

import re
from dataclasses import dataclass, field

from command_normalizer import correct_stt_errors, normalize_vietnamese_text

# Backward compat: ten normalize_text van ton tai (TANG 1 pipeline AC).
normalize_text = normalize_vietnamese_text

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
    "AIR_AUTO": 11,
}

TEMP_MIN = 18
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
# NORMALIZE (TANG 1 + TANG 3 — xem command_normalizer.py)
# =========================================================

# normalize_text = TANG 1 (NFD bo dau + d->d + lowercase + dau cau + space)
# correct_stt_errors = TANG 3 (loi STT/vung mien phrase-level, gate thiet bi)


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

# So chu tieng Viet 0-99 + dang rut gon vung mien/STT:
#   "hai muoi lam" / "hai lam" (mien Nam, STT bo sot "muoi") -> 25
#   "hai muoi tu"  / "hai tu"   -> 24
#   "hai ba"/"hai sau"/"hai bay"/"hai tam"/"hai chin" (STT bo sot "muoi") -> 23..29
# "hai" don le van = 2 ("hai do" -> 2, khong phai 20). "hai muoi" uu tien TRUOC.
_WORD_NUM_RE = re.compile(
    r"(?<![a-z])(?:"
    r"(?:hai|ba|bon|nam|sau|bay|tam|chin)\s+muoi"
    r"(?:\s+(?:lam|nam|tu|mot|hai|ba|bon|sau|bay|tam|chin|khong))?"
    r"|muoi(?:\s+(?:lam|nam|tu|mot|hai|ba|bon|sau|bay|tam|chin|khong))?"
    r"|hai\s+(?:lam|nam|tu|bon|ba|sau|bay|tam|chin)"
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
    parts = s.split()
    if len(parts) == 2 and parts[0] == "hai":
        # "hai lam" = 25 / "hai tu" = 24 / "hai ba" = 23 (khong co "muoi")
        return 20 + _UNIT_AFTER_MUOI[parts[1]]
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

_NEGATION_TOKENS = {"khong", "chua", "dung"}  # "dung" = đừng khi dung truoc dong tu
_COMMAND_VERBS = frozenset(
    {
        "mo", "bat", "chay", "khoi dong",
        "tat", "dong", "ngung", "ngat", "dung",
        "tang", "giam", "len", "xuong", "ha", "bot",
        "dat", "chinh", "cai", "de", "muon", "cho",
    }
)

# =========================================================
# AC / FAN: CONTEXT REQUIREMENT (dong tu + device)
# =========================================================

_AC_DEVICE_PHRASES = ("dieu hoa", "may lanh", "may dieu hoa")
_FAN_DEVICE_PHRASES = ("quat",)

_AC_ON_VERBS = ("mo", "bat", "chay", "khoi dong")
_AC_OFF_VERBS = ("tat", "dong", "ngung", "ngat", "dung")
_FAN_ON_VERBS = ("mo", "bat")
_FAN_OFF_VERBS = ("tat", "ngung", "ngat", "dung")

# =========================================================
# KEYWORD SCORING (TEMP_UP/DOWN + AIR + HELLO)
# =========================================================

_INTENT_KEYWORDS: dict[str, dict[str, int]] = {
    "HELLO": {
        "xin chao": 2, "hello": 2, "chao ban": 2, "chao": 1,
    },
    "GOODBYE": {
        "tam biet": 2, "chao tam biet": 2, "bye": 2, "goodbye": 2,
        "hen gap lai": 2,
    },
    # TEMP_UP = NGUOI DUNG MUON NONG HON / DANG LANH:
    #   dieu chinh: tang/nong hon/nong len (giu 4)
    #   cam nhan lanh: lanh qua / toi lanh / lanh (4)
    "TEMP_UP": {
        "tang": 2, "tang nhiet": 2, "tang len": 2,
        "nhiet do": 1, "nong hon": 3, "nong len": 3,
        "am hon": 2, "am len": 2, "len": 1,
        "lanh qua": 3, "toi lanh": 3, "lanh": 2,
    },
    # TEMP_DOWN = NGUOI DUNG MUON LANH HON / DANG NONG:
    #   dieu chinh: giam/lanh hon/lanh di (giu 5)
    #   cam nhan nong: nong qua / toi nong / nong (5)
    "TEMP_DOWN": {
        "giam": 2, "giam nhiet": 2, "giam xuong": 2,
        "nhiet do": 1, "lanh hon": 3, "lanh di": 3,
        "ha nhiet": 2, "bot nhiet": 2, "xuong": 1,
        "nong qua": 3, "toi nong": 3, "nong": 2,
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

# Context hop le cho SET/INVALID_TEMPERATURE:
# - "nhiet" HOAC device dieu hoa/may lanh: luon du.
# - dong tu yeu de/muon/cho: du (dung theo danh sach context da duyet).
# - dat/chinh/cai: chi du khi cau CON co "do" ("dat 25 do" OK;
#   "dat hang 25 cai"/"mua 25 cai" KHONG -> "cai" trung voi classifier "cai").
# Chi co "<so> + do" la CHUA DU dieu kien ("25 do ngoai troi" -> UNKNOWN).
_WEAK_SET_VERBS = ("de", "muon", "cho")
_VERB_WITH_DO = ("dat", "chinh", "cai")

# TEMP_UP/DOWN: neu thang chi bang 1 tu don yeu (tang/giam/len/xuong) thi
# BAT BUOC cau co token ngua canh nhiet do, khong thi UNKNOWN.
# (chong false positive "tang qua" -> TEMP_UP; van giu "giam nhiep do" -> 5 vi co "do")
_TEMP_WEAK_VERBS = {"tang", "giam", "len", "xuong"}
_TEMP_CTX_TOKENS = {
    "nhiet", "do", "len", "xuong", "nong", "lanh", "am",
    "hon", "ha", "bot",
}


def _weak_verb_needs_context(matched: list[str], tokens: list[str]) -> bool:
    """True = thang bang 1 tu don yeu ma thieu ngua canh nhiet do (chan lay)."""
    if len(matched) == 1 and matched[0] in _TEMP_WEAK_VERBS:
        return not any(t in _TEMP_CTX_TOKENS for t in tokens)
    return False


def _is_air_auto(normalized: str, tokens: list[str]) -> bool:
    """AIR_AUTO = cung 1 cau co CA "mat" VA "chan" + ngua canh huong gio.

    Rule nay phai chay TRUOC AIR_FACE/AIR_FOOT de khong cho AIR_FOOT
    thang chi vi "chan" match truoc.
    """
    if "mat" not in tokens or "chan" not in tokens:
        return False
    return any(
        _phrase_matches(normalized, p) for p in ("huong gio", "thoi", "gio")
    )

# =========================================================
# RESPONSES
# =========================================================

_RESPONSES = {
    "AC_ON": "Đã bật điều hòa.",
    "AC_OFF": "Đã tắt điều hòa.",
    "TEMP_UP": "Đã tăng nhiệt độ lên hai độ.",
    "TEMP_DOWN": "Đã giảm nhiệt độ xuống hai độ.",
    "FAN_ON": "Đã bật quạt.",
    "FAN_OFF": "Đã tắt quạt.",
    "AIR_FACE": "Đã chuyển hướng gió lên mặt.",
    "AIR_FOOT": "Đã chuyển hướng gió xuống chân.",
    "AIR_DEFROST": "Đã bật chế độ sưởi kính chắn gió.",
    "AIR_AUTO": "Đã chuyển sang chế độ gió tự động.",
    "HELLO": "Xin chào! Tôi đang hoạt động.",
    "GOODBYE": "Tạm biệt! Hẹn gặp lại.",
    "UNKNOWN": "Tôi chưa hiểu yêu cầu của bạn.",
    "INVALID_TEMPERATURE": "Nhiệt độ cho phép từ 18 đến 30 độ.",
    "SET_TEMPERATURE": "Điều hoà đã đặt theo yêu cầu của bạn.",
}


# =========================================================
# CLASSIFY
# =========================================================

def classify(raw_text: str) -> AIResult:
    """Phan loai cau noi -> AIResult (deterministic, local, khong AI/API).

    Thu tu uu tien:
      NEGATION GATE -> NHIET DO CU THE -> TANG/GIAM -> SET khong co so
      -> TU DONG/AUTO -> AC/FAN (context) -> KEYWORD SCORING (AIR/HELLO/GOODBYE)
      -> UNKNOWN
    """

    normalized = correct_stt_errors(normalize_text(raw_text))
    tokens = _tokenize(normalized)
    word_count = max(1, len(tokens))

    def make(intent: str, code: int | None, temp: int | None,
             conf: float, matched: list[str]) -> AIResult:
        response = _RESPONSES.get(intent, _RESPONSES["UNKNOWN"])
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
    # "dung" (đừng) la phu dinh CHI khi dung TRUOC dong tu ("dung bat dieu hoa").
    # "dung" + device ("dung dieu hoa" = dừng) la lenh TAT -> de B4 xu ly.
    for i, t in enumerate(tokens):
        if t in _NEGATION_TOKENS:
            if t == "dung" and not (
                i + 1 < len(tokens) and tokens[i + 1] in _COMMAND_VERBS
            ):
                continue
            return make(
                "UNKNOWN", None, None, UNKNOWN_CONFIDENCE, [f"phu-dinh={t}"]
            )

    # ---- B1: NHIET DO CU THE (uu tien nhat) ----
    # Context BAT BUOC: "nhiet" HOAC device dieu hoa/may lanh HOAC dong tu
    # dat/chinh/cai/de/muon/cho. CHI co "<so> + do" KHONG du dieu kien
    # ("25 do ngoai troi" -> UNKNOWN, khong phai lenh dieu hoa).
    numbers = _extract_numbers(normalized)

    def _has_temp_context() -> bool:
        if "nhiet" in normalized:
            return True
        if any(_phrase_matches(normalized, d) for d in _AC_DEVICE_PHRASES):
            return True
        if any(t in _WEAK_SET_VERBS for t in tokens):
            return True
        if any(t in _VERB_WITH_DO for t in tokens) and "do" in tokens:
            return True
        return False

    if numbers and _has_temp_context():
        for n in numbers:
            if TEMP_MIN <= n <= TEMP_MAX:
                return make(
                    "SET_TEMPERATURE", TEMP_BASE + n, n,
                    SET_CONFIDENCE, [f"so={n}", "context=temp-hop-le"],
                )
        # Co so nhung ngoai 18-30 -> INVALID, giu lai gia tri de hien thi
        return make(
            "INVALID_TEMPERATURE", None, numbers[0],
            INVALID_CONFIDENCE, [f"so={numbers[0]}", "ngoai-khoang-18-30"],
        )

    # ---- B2: TANG/GIAM NHIET DO (uu tien hon con so don le) ----
    up_score, up_kw = _score_keywords(normalized, _INTENT_KEYWORDS["TEMP_UP"])
    down_score, down_kw = _score_keywords(normalized, _INTENT_KEYWORDS["TEMP_DOWN"])

    # "may lanh" = device (AC), khong phai cam nhan lanh:
    # loai bo bare "lanh" khoi TEMP_UP ("mo may lanh" -> AC_ON, khong phai TEMP_UP).
    if _phrase_matches(normalized, "may lanh") and "lanh" in up_kw:
        up_score -= _INTENT_KEYWORDS["TEMP_UP"]["lanh"]
        up_kw = [k for k in up_kw if k != "lanh"]

    if up_score >= MIN_SCORE and not _weak_verb_needs_context(up_kw, tokens) \
            and up_score >= down_score:
        return make("TEMP_UP", COMMAND_CODES["TEMP_UP"], None,
                    round(min(1.0, up_score / word_count), 2), up_kw)
    if down_score >= MIN_SCORE and not _weak_verb_needs_context(down_kw, tokens) \
            and down_score > up_score:
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

    # ---- B3.5: TU DONG / AUTO -> AIR_AUTO ----
    # "tu dong" (tu dong) / "auto" = che do gio tu dong.
    # Phai chay TRUOC B4: "tu dong dieu hoa" co token "dong" trung voi dong tu
    # "dong" (đóng) cua AC_OFF -> neu de B4 lo, se bi nhan nham thanh AC_OFF.
    # Negation da bi chan o B0 nen khong can lo "khong tu dong".
    if _phrase_matches(normalized, "tu dong") or _phrase_matches(normalized, "auto"):
        return make(
            "AIR_AUTO", COMMAND_CODES["AIR_AUTO"], None,
            round(min(1.0, 3 / word_count), 2), ["tu-dong/auto"],
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
        # "quạt lên" (chi trang tu, khong co dong tu) -> FAN_ON.
        # "quạt" dung mot minh van UNKNOWN ("len" bat buoc phai co).
        if "len" in tokens:
            return make("FAN_ON", COMMAND_CODES["FAN_ON"], None,
                        round(min(1.0, 4 / word_count), 2), ["len", fan_device])

    # ---- B5: KEYWORD SCORING (AIR / HELLO / GOODBYE) ----
    # B5a: AIR_AUTO uu tien TRUOC AIR_FACE/AIR_FOOT ("mat" + "chan" cung cau)
    if _is_air_auto(normalized, tokens):
        return make(
            "AIR_AUTO", COMMAND_CODES["AIR_AUTO"], None,
            round(min(1.0, 4 / word_count), 2), ["mat+chan=auto"],
        )

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
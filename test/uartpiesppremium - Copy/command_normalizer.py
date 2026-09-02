"""Command normalization cho pipeline Vosk -> ESP32.

Chuan hoa tieng Viet da tang (LOCAL, deterministic, khong AI/API):

TANG 1 - Basic cleanup:
    NFD tach dau, bo combining marks, lowercase, bo dau cau, gop khoang trang.
    - normalize_text():           giu NGUYEN hanh vi cu (robot commands:
                                  "dung" vs "dung" la 2 tu khac nhau).
    - normalize_vietnamese_text(): bo sung "d"->"d" (U+0111) cho pipeline AC
                                  (Vosk co the tra "d" hoac "d" cho cung 1 tu).

TANG 2 - Tu dong nghia:
    canonicalize(): gop cac cum tu dong nghia ve 1 dang canonical
    (word boundary, cum dai truoc). Dung cho hien thi NORMALIZED trong debug.

TANG 3 - Loi STT / vung mien:
    correct_stt_errors(): sua cac loi phat am/nhan dang pho bien con sot lai
    sau TANG 1 (thanh dieu/nguyen am da gop het o TANG 1; o day chi con
    phu am dau/cuoi bi doi). CHI phrase-level, GATED boi tu thiet bi
    (dieu hoa/may lanh/quat/suoi kinh) de han che false positive.
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

_PUNCTUATION_RE = re.compile(r"[\.,!?;:\"']+")


def _normalize_basic(text: str) -> str:
    """TANG 1 core: NFD + bo dau + lowercase + bo dau cau + gop space."""
    text = unicodedata.normalize("NFD", text)
    text = "".join(ch for ch in text if unicodedata.category(ch) != "Mn")
    text = text.lower()
    text = _PUNCTUATION_RE.sub(" ", text)
    text = " ".join(text.split())
    return text.strip()


def normalize_text(text: str) -> str:
    """TANG 1 (robot commands): bo dau tieng Viet, lowercase, chuan hoa space.

    KHONG map 'd' -> 'd' o day: "dung" (dung) phai giu khac "dung" (dung).
    """
    return _normalize_basic(text)


def normalize_vietnamese_text(text: str) -> str:
    """TANG 1 (pipeline AC): nhu normalize_text + 'd' -> 'd'.

    Vosk co the tra cung 1 tu voi "d" hoac "d" ("dieu hoa" / "dieu hoa");
    pipeline dieu hoa can gop chung ve "d".
    """
    if not text:
        return ""
    normalized = _normalize_basic(text)
    return normalized.replace("đ", "d")  # U+0111 khong bi NFD phan ra


# =========================================================
# TANG 2 - TU DONG NGHIA (canonical)
# =========================================================

# key = canonical, values = cac bien the (duoc thay thanh canonical).
# THU TU QUAN TRONG: cum dai hon phai dung TRUOC ("may dieu hoa" > "may lanh").
_SYNONYM_GROUPS: dict[str, list[str]] = {
    "dieu hoa": ["may dieu hoa", "dieu hoa", "may lanh"],
}


def _synonym_rules() -> list[tuple[re.Pattern, str]]:
    rules: list[tuple[re.Pattern, str]] = []
    for canonical, variants in _SYNONYM_GROUPS.items():
        for v in sorted(variants, key=len, reverse=True):
            pattern = re.compile(
                r"(?<![a-z0-9])" + re.escape(v) + r"(?![a-z0-9])"
            )
            rules.append((pattern, canonical))
    return rules


_SYNONYM_RULES = _synonym_rules()


def canonicalize(text: str) -> str:
    """TANG 2: thay cac cum dong nghia ve dang canonical (word boundary).

    CHI dung cho hien thi/debug (NORMALIZED), KHONG dung cho matching:
    classifier giu phrase lists rieng de kiem soat context.
    """
    if not text:
        return text
    result = text
    for pattern, canonical in _SYNONYM_RULES:
        result = pattern.sub(canonical, result)
    return result


# =========================================================
# TANG 3 - LOI STT / VUNG MIEN (phrase-level, gated boi thiet bi)
# =========================================================

# Sau TANG 1, thanh dieu + nguyen am da gop het ("bat"/"bac"/"bat" -> "bat").
# Chi con sot cac doi phu am dau/cuoi. Tat ca rule deu YEU CAU cum tu thiet bi
# di kem (hoac cum co nghia AC ro) de false positive thap.
_STT_ERROR_RULES: list[tuple[str, str]] = [
    # "bạc điều hòa" (bật -> bạc, t -> c) -> "bật điều hòa"
    (r"\bbac\s+(dieu hoa|may lanh|quat|suoi kinh)", r"bat \1"),
    # "máy lạng" (máy lạnh, n -> ng, giong Nam) -> "máy lạnh"
    (r"\bmay\s+lang\b", "may lanh"),
    # "tắc điều hòa" (tắt -> tắc, t -> c) -> "tắt điều hòa"
    (r"\btac\s+(dieu hoa|may lanh|quat)", r"tat \1"),
    # "quạc lên" (quạt -> quạc, t -> c) -> "quạt lên"
    (r"\bquac\s+len\b", "quat len"),
]

_STT_ERROR_COMPILED: list[tuple[re.Pattern, str]] = [
    (re.compile(pattern), replacement) for pattern, replacement in _STT_ERROR_RULES
]


def correct_stt_errors(text: str) -> str:
    """TANG 3: sua loi STT/vung mien con sot sau TANG 1.

    Input phai la text da normalize (ASCII lowercase). Chi ap dung cac rule
    phrase-level co gate thiet bi. Tra text da sua, khong doi nhung gi khac.
    """
    if not text:
        return text
    result = text
    for pattern, replacement in _STT_ERROR_COMPILED:
        result = pattern.sub(replacement, result)
    return result


def normalize_command(text: str | None) -> str | None:
    """Map text thanh command canonical (HELLO/FORWARD/...).

    Tra ve None neu khong phai command hop le (EXACT match).
    """
    if not text:
        return None
    key = normalize_text(text)
    return _COMMAND_MAP.get(key)
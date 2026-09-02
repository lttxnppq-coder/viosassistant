"""test_command_ai.py - test command_ai.py (rule-based classifier).

Khong can microphone/Vosk/ESP32/Piper.

Chay:  python test_command_ai.py
"""

import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

sys.path.insert(0, str(Path(__file__).resolve().parent))

from command_ai import classify

# (input, expected_intent, expected_code, expected_temp)
CASES: list[tuple[str, str, int | None, int | None]] = [
    # ---- NHOM 1: AC_ON ----
    ("mở điều hòa", "AC_ON", 1, None),
    ("bat dieu hoa", "AC_ON", 1, None),
    ("mo may lanh", "AC_ON", 1, None),
    ("bật máy lạnh", "AC_ON", 1, None),
    ("cho dieu hoa chay", "AC_ON", 1, None),
    ("mo dieu hoa len", "AC_ON", 1, None),
    ("khởi động điều hòa", "AC_ON", 1, None),
    ("khoi dong dieu hoa", "AC_ON", 1, None),
    ("bật điều hoà", "AC_ON", 1, None),
    ("mở điều hoà", "AC_ON", 1, None),
    ("mở máy lạnh", "AC_ON", 1, None),
    ("bật điều hòa lên", "AC_ON", 1, None),
    ("mở điều hòa lên", "AC_ON", 1, None),
    ("cho điều hoà chạy", "AC_ON", 1, None),
    ("khởi động điều hoà", "AC_ON", 1, None),
    # ---- NHOM 2: AC_OFF ----
    ("tắt điều hòa", "AC_OFF", 2, None),
    ("tat may lanh", "AC_OFF", 2, None),
    ("đóng điều hòa", "AC_OFF", 2, None),
    ("ngat dieu hoa", "AC_OFF", 2, None),
    ("tắt điều hoà", "AC_OFF", 2, None),
    ("đóng điều hoà", "AC_OFF", 2, None),
    ("đóng máy lạnh", "AC_OFF", 2, None),
    ("tắt điều hòa đi", "AC_OFF", 2, None),
    ("dừng điều hòa", "AC_OFF", 2, None),
    ("dừng điều hoà", "AC_OFF", 2, None),
    # ---- NHOM 3: TEMP_UP (dieu chinh nong len / cam nhan lanh) ----
    ("tang nhiet do", "TEMP_UP", 4, None),
    ("tăng lên một chút", "TEMP_UP", 4, None),
    ("cho nóng lên", "TEMP_UP", 4, None),
    ("cho am hon", "TEMP_UP", 4, None),
    ("lạnh quá", "TEMP_UP", 4, None),
    ("lanh qua", "TEMP_UP", 4, None),
    ("lạnh quá mẹ ơi", "TEMP_UP", 4, None),
    ("tôi lạnh", "TEMP_UP", 4, None),
    ("lạnh", "TEMP_UP", 4, None),
    ("nhiệt độ lạnh quá", "TEMP_UP", 4, None),
    ("lạnh quá rồi", "TEMP_UP", 4, None),
    ("lạnh quá, tăng nhiệt độ", "TEMP_UP", 4, None),
    # ---- NHOM 4: TEMP_DOWN (dieu chinh lanh di / cam nhan nong) ----
    ("giam nhiet do", "TEMP_DOWN", 5, None),
    ("cho lạnh hơn", "TEMP_DOWN", 5, None),
    ("hạ nhiệt độ", "TEMP_DOWN", 5, None),
    ("giam xuong mot chut", "TEMP_DOWN", 5, None),
    ("nóng quá", "TEMP_DOWN", 5, None),
    ("nong qua", "TEMP_DOWN", 5, None),
    ("nóng quá mẹ ơi", "TEMP_DOWN", 5, None),
    ("tôi nóng", "TEMP_DOWN", 5, None),
    ("nóng", "TEMP_DOWN", 5, None),
    ("nhiệt độ nóng quá", "TEMP_DOWN", 5, None),
    ("nóng quá rồi", "TEMP_DOWN", 5, None),
    ("nóng quá, giảm nhiệt độ", "TEMP_DOWN", 5, None),
    ("tăng nhiệt độ", "TEMP_UP", 4, None),
    ("giảm nhiệt độ", "TEMP_DOWN", 5, None),
    # ---- NHOM 5: FAN_ON ----
    ("mo quat", "FAN_ON", 6, None),
    ("bat quat", "FAN_ON", 6, None),
    # ---- NHOM 6: FAN_OFF ----
    ("tắt quạt", "FAN_OFF", 7, None),
    ("ngat quat", "FAN_OFF", 7, None),
    ("dừng quạt", "FAN_OFF", 7, None),
    # ---- NHOM 7: AIR_FACE ----
    ("gio len mat", "AIR_FACE", 8, None),
    ("hướng gió lên mặt", "AIR_FACE", 8, None),
    ("thoi vao mat", "AIR_FACE", 8, None),
    ("thổi lên mặt", "AIR_FACE", 8, None),
    ("thoi len mat", "AIR_FACE", 8, None),
    ("hướng gió ra mặt", "AIR_FACE", 8, None),
    # ---- NHOM 8: AIR_FOOT ----
    ("gio xuong chan", "AIR_FOOT", 9, None),
    ("hướng gió xuống chân", "AIR_FOOT", 9, None),
    ("thoi xuong chan", "AIR_FOOT", 9, None),
    ("thổi xuống chân", "AIR_FOOT", 9, None),
    # ---- NHOM 7b: AIR_AUTO (ca "mat" + "chan" trong cung 1 cau) ----
    ("hướng gió xuống chân và mặt", "AIR_AUTO", 11, None),
    ("hướng gió mặt và chân", "AIR_AUTO", 11, None),
    ("thổi cả mặt và chân", "AIR_AUTO", 11, None),
    ("huong gio mat va chan", "AIR_AUTO", 11, None),
    ("gió cả mặt và chân", "AIR_AUTO", 11, None),
    ("cho gió xuống chân và mặt", "AIR_AUTO", 11, None),
    # ---- NHOM 9: AIR_DEFROST ----
    ("gio vao kinh", "AIR_DEFROST", 10, None),
    ("thoi kinh", "AIR_DEFROST", 10, None),
    ("say kinh", "AIR_DEFROST", 10, None),
    ("huong gio suoi kinh chan gio", "AIR_DEFROST", 10, None),
    # ---- NHOM 10: SET_TEMPERATURE (chu so) ----
    ("đặt nhiệt độ 23 độ", "SET_TEMPERATURE", 323, 23),
    ("đặt nhiệt độ 25 độ", "SET_TEMPERATURE", 325, 25),
    ("dat nhiet do 25", "SET_TEMPERATURE", 325, 25),
    ("đặt nhiệt độ 30 độ", "SET_TEMPERATURE", 330, 30),
    ("chỉnh điều hòa xuống 24 độ", "SET_TEMPERATURE", 324, 24),
    ("cho may lanh 27 do", "SET_TEMPERATURE", 327, 27),
    ("để 28 độ", "SET_TEMPERATURE", 328, 28),
    ("toi muon 30 do", "SET_TEMPERATURE", 330, 30),
    ("cho điều hòa 28 độ", "SET_TEMPERATURE", 328, 28),
    ("cho dieu hoa 28 do", "SET_TEMPERATURE", 328, 28),
    # ---- NHOM 10c: SET_TEMPERATURE (so chu + device context) ----
    ("đặt điều hòa 23 độ", "SET_TEMPERATURE", 323, 23),
    ("đặt điều hòa 24 độ", "SET_TEMPERATURE", 324, 24),
    ("đặt điều hòa 25 độ", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hòa 26 độ", "SET_TEMPERATURE", 326, 26),
    ("đặt điều hòa 27 độ", "SET_TEMPERATURE", 327, 27),
    ("đặt điều hòa 28 độ", "SET_TEMPERATURE", 328, 28),
    ("đặt điều hòa 29 độ", "SET_TEMPERATURE", 329, 29),
    ("đặt điều hòa 30 độ", "SET_TEMPERATURE", 330, 30),
    ("nhiệt độ 25 độ", "SET_TEMPERATURE", 325, 25),
    ("điều hòa 25 độ", "SET_TEMPERATURE", 325, 25),
    ("cho điều hòa 25 độ", "SET_TEMPERATURE", 325, 25),
    ("hãy đặt nhiệt độ 27 độ", "SET_TEMPERATURE", 327, 27),
    # ---- NHOM 10d: SET_TEMPERATURE (so chu tieng Viet + device) ----
    ("đặt điều hòa hai mươi ba độ", "SET_TEMPERATURE", 323, 23),
    ("đặt điều hòa hai mươi bốn độ", "SET_TEMPERATURE", 324, 24),
    ("đặt điều hòa hai mươi lăm độ", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hòa hai mươi sáu độ", "SET_TEMPERATURE", 326, 26),
    ("đặt điều hòa hai mươi bảy độ", "SET_TEMPERATURE", 327, 27),
    ("đặt điều hòa hai mươi tám độ", "SET_TEMPERATURE", 328, 28),
    ("đặt điều hòa hai mươi chín độ", "SET_TEMPERATURE", 329, 29),
    ("đặt điều hòa ba mươi độ", "SET_TEMPERATURE", 330, 30),
    # ---- NHOM 10e: SET_TEMPERATURE (ASR noise cuoi cau) ----
    ("đặt điều hòa hai mươi lăm độ xe", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hòa hai mươi bốn đồ đệ", "SET_TEMPERATURE", 324, 24),
    ("đặt điều hòa hai mươi bảy đỗ xe", "SET_TEMPERATURE", 327, 27),
    # ---- NHOM 10b: SET_TEMPERATURE (so chu tieng Viet) ----
    ("đặt nhiệt độ hai mươi lăm độ", "SET_TEMPERATURE", 325, 25),
    ("dat nhiet do hai muoi ba do", "SET_TEMPERATURE", 323, 23),
    ("dat nhiet do ba muoi do", "SET_TEMPERATURE", 330, 30),
    # ---- NHOM 10f: SET_TEMPERATURE day du 18-30 ----
    ("đặt nhiệt độ 18 độ", "SET_TEMPERATURE", 318, 18),
    ("đặt nhiệt độ 19 độ", "SET_TEMPERATURE", 319, 19),
    ("đặt nhiệt độ 20 độ", "SET_TEMPERATURE", 320, 20),
    ("đặt nhiệt độ 21 độ", "SET_TEMPERATURE", 321, 21),
    ("đặt nhiệt độ 22 độ", "SET_TEMPERATURE", 322, 22),
    ("đặt nhiệt độ 24 độ", "SET_TEMPERATURE", 324, 24),
    ("đặt nhiệt độ 26 độ", "SET_TEMPERATURE", 326, 26),
    ("đặt nhiệt độ 27 độ", "SET_TEMPERATURE", 327, 27),
    ("đặt nhiệt độ 28 độ", "SET_TEMPERATURE", 328, 28),
    ("đặt nhiệt độ 29 độ", "SET_TEMPERATURE", 329, 29),
    ("cho 19 do", "SET_TEMPERATURE", 319, 19),
    ("đặt điều hòa hai mươi hai độ", "SET_TEMPERATURE", 322, 22),
    ("đặt điều hòa hai mươi mốt độ", "SET_TEMPERATURE", 321, 21),
    ("đặt nhiệt độ hai mươi độ", "SET_TEMPERATURE", 320, 20),
    ("đặt nhiệt độ mười tám độ", "SET_TEMPERATURE", 318, 18),
    # ---- NHOM 10g: SET_TEMPERATURE bien the cau noi (18/20/23/25/28/30) ----
    ("đặt điều hòa 18 độ", "SET_TEMPERATURE", 318, 18),
    ("đặt điều hoà 18 độ", "SET_TEMPERATURE", 318, 18),
    ("chỉnh nhiệt độ 18 độ", "SET_TEMPERATURE", 318, 18),
    ("chỉnh điều hòa 18 độ", "SET_TEMPERATURE", 318, 18),
    ("cho điều hòa 18 độ", "SET_TEMPERATURE", 318, 18),
    ("để điều hòa 18 độ", "SET_TEMPERATURE", 318, 18),
    ("đặt nhiệt độ ở 18 độ C", "SET_TEMPERATURE", 318, 18),
    ("đặt điều hòa ở 18 độ C", "SET_TEMPERATURE", 318, 18),
    ("đặt điều hòa 20 độ", "SET_TEMPERATURE", 320, 20),
    ("đặt điều hoà 20 độ", "SET_TEMPERATURE", 320, 20),
    ("chỉnh nhiệt độ 20 độ", "SET_TEMPERATURE", 320, 20),
    ("chỉnh điều hòa 20 độ", "SET_TEMPERATURE", 320, 20),
    ("cho điều hòa 20 độ", "SET_TEMPERATURE", 320, 20),
    ("để điều hòa 20 độ", "SET_TEMPERATURE", 320, 20),
    ("đặt nhiệt độ ở 20 độ C", "SET_TEMPERATURE", 320, 20),
    ("đặt điều hòa ở 20 độ C", "SET_TEMPERATURE", 320, 20),
    ("đặt điều hoà 23 độ", "SET_TEMPERATURE", 323, 23),
    ("chỉnh nhiệt độ 23 độ", "SET_TEMPERATURE", 323, 23),
    ("chỉnh điều hòa 23 độ", "SET_TEMPERATURE", 323, 23),
    ("cho điều hòa 23 độ", "SET_TEMPERATURE", 323, 23),
    ("để điều hòa 23 độ", "SET_TEMPERATURE", 323, 23),
    ("đặt nhiệt độ ở 23 độ C", "SET_TEMPERATURE", 323, 23),
    ("đặt điều hòa ở 23 độ C", "SET_TEMPERATURE", 323, 23),
    ("đặt điều hoà 25 độ", "SET_TEMPERATURE", 325, 25),
    ("chỉnh nhiệt độ 25 độ", "SET_TEMPERATURE", 325, 25),
    ("chỉnh điều hòa 25 độ", "SET_TEMPERATURE", 325, 25),
    ("để điều hòa 25 độ", "SET_TEMPERATURE", 325, 25),
    ("đặt nhiệt độ ở 25 độ C", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hòa ở 25 độ C", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hoà 28 độ", "SET_TEMPERATURE", 328, 28),
    ("chỉnh nhiệt độ 28 độ", "SET_TEMPERATURE", 328, 28),
    ("chỉnh điều hòa 28 độ", "SET_TEMPERATURE", 328, 28),
    ("để điều hòa 28 độ", "SET_TEMPERATURE", 328, 28),
    ("đặt nhiệt độ ở 28 độ C", "SET_TEMPERATURE", 328, 28),
    ("đặt điều hòa ở 28 độ C", "SET_TEMPERATURE", 328, 28),
    ("đặt điều hoà 30 độ", "SET_TEMPERATURE", 330, 30),
    ("chỉnh nhiệt độ 30 độ", "SET_TEMPERATURE", 330, 30),
    ("chỉnh điều hòa 30 độ", "SET_TEMPERATURE", 330, 30),
    ("cho điều hòa 30 độ", "SET_TEMPERATURE", 330, 30),
    ("để điều hòa 30 độ", "SET_TEMPERATURE", 330, 30),
    ("đặt nhiệt độ ở 30 độ C", "SET_TEMPERATURE", 330, 30),
    ("đặt điều hòa ở 30 độ C", "SET_TEMPERATURE", 330, 30),
    # ---- NHOM 10h: regression (so chu, bien the cau noi, multi-temp) ----
    ("đặt nhiệt độ mười chín độ", "SET_TEMPERATURE", 319, 19),
    ("set nhiệt độ 25 độ", "SET_TEMPERATURE", 325, 25),
    ("chỉnh điều hoà 25 độ", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hoà ở 25 độ C", "SET_TEMPERATURE", 325, 25),
    ("đặt máy lạnh 26 độ", "SET_TEMPERATURE", 326, 26),
    ("đặt nhiệt độ mười tám độ C", "SET_TEMPERATURE", 318, 18),
    ("cho điều hoà 18 độ", "SET_TEMPERATURE", 318, 18),
    ("đặt nhiệt độ 16 độ và 25 độ", "SET_TEMPERATURE", 325, 25),
    ("đặt nhiệt độ 25 độ và 31 độ", "SET_TEMPERATURE", 325, 25),
    ("hãy đặt nhiệt độ", "UNKNOWN", None, None),
    # ---- NHOM 11: INVALID_TEMPERATURE (< 18 hoac > 30) ----
    ("đặt nhiệt độ 16 độ", "INVALID_TEMPERATURE", None, 16),
    ("đặt nhiệt độ 17 độ", "INVALID_TEMPERATURE", None, 17),
    ("đặt nhiệt độ 31 độ", "INVALID_TEMPERATURE", None, 31),
    ("đặt nhiệt độ 32 độ", "INVALID_TEMPERATURE", None, 32),
    ("đặt nhiệt độ 35 độ", "INVALID_TEMPERATURE", None, 35),
    ("đặt điều hòa 40 độ", "INVALID_TEMPERATURE", None, 40),
    ("dat nhiet do nay lam ho so thue", "INVALID_TEMPERATURE", None, None),
    ("đặt điều hòa ba mươi mốt độ", "INVALID_TEMPERATURE", None, 31),
    ("đặt nhiệt độ mười bảy độ", "INVALID_TEMPERATURE", None, 17),
    ("đặt nhiệt độ ba mươi mốt độ", "INVALID_TEMPERATURE", None, 31),
    # ---- NHOM 12: UNKNOWN ----
    ("điều hòa", "UNKNOWN", None, None),
    ("hom nay troi dep", "UNKNOWN", None, None),
    ("toi muon di choi", "UNKNOWN", None, None),
    ("chay", "UNKNOWN", None, None),
    ("abcxyz", "UNKNOWN", None, None),
    # ---- NHOM 13: HELLO ----
    ("xin chào", "HELLO", None, None),
    ("hello", "HELLO", None, None),
    # ---- NHOM 13b: GOODBYE ----
    ("tạm biệt", "GOODBYE", None, None),
    ("chào tạm biệt", "GOODBYE", None, None),
    ("bye", "GOODBYE", None, None),
    ("goodbye", "GOODBYE", None, None),
    ("hẹn gặp lại", "GOODBYE", None, None),
    # ---- NHOM 14: VOSK GARBAGE / FALSE POSITIVE -> UNKNOWN ----
    ("bất quá", "UNKNOWN", None, None),
    ("bat qua", "UNKNOWN", None, None),
    ("tổng quát", "UNKNOWN", None, None),
    ("hoặc quát", "UNKNOWN", None, None),
    ("do lên phát", "UNKNOWN", None, None),
    ("do lê hát", "UNKNOWN", None, None),
    ("góc nhìn", "UNKNOWN", None, None),
    ("góc nhiệt độ hoa ngữ làm ruộng xe", "UNKNOWN", None, None),
    ("bat den", "UNKNOWN", None, None),
    ("bat may", "UNKNOWN", None, None),
    ("mo cua", "UNKNOWN", None, None),
    ("mo den", "UNKNOWN", None, None),
    ("tat den", "UNKNOWN", None, None),
    ("tat may", "UNKNOWN", None, None),
    ("dong cua", "UNKNOWN", None, None),
    ("gió", "UNKNOWN", None, None),
    ("nhiệt độ", "UNKNOWN", None, None),
    ("dat nhiet do hoac ruot russia", "UNKNOWN", None, None),
    ("do lac o mot", "UNKNOWN", None, None),
    ("da dieu hoa phu", "UNKNOWN", None, None),
    ("moc quat", "UNKNOWN", None, None),
    ("dat quet", "UNKNOWN", None, None),
    ("giai nhiet doc", "UNKNOWN", None, None),
    ("doanh nghiep dia oc", "UNKNOWN", None, None),
    ("at deu hoa", "UNKNOWN", None, None),
    # ---- NHOM 12b: NO TEMP CONTEXT -> UNKNOWN (khong de dai) ----
    ("25 độ ngoài trời", "UNKNOWN", None, None),
    ("tôi có 25 quyển sách", "UNKNOWN", None, None),
    ("hôm nay là 27", "UNKNOWN", None, None),
    ("mua 25 cái", "UNKNOWN", None, None),
    # ---- NHOM 12c: WEAK VERB THIEU CONTEXT -> UNKNOWN (false positive) ----
    ("tặng quà", "UNKNOWN", None, None),
    ("tang qua", "UNKNOWN", None, None),
    ("tang tien", "UNKNOWN", None, None),
    # ---- NHOM 15: NEGATION (phu dinh) -> UNKNOWN ----
    ("khong bat dieu hoa", "UNKNOWN", None, None),
    ("dung bat dieu hoa", "UNKNOWN", None, None),
    ("dung tat dieu hoa", "UNKNOWN", None, None),
    ("khong tat quat", "UNKNOWN", None, None),
    ("mở điều hòa không", "UNKNOWN", None, None),
    ("chua bat dieu hoa", "UNKNOWN", None, None),
    # ---- NHOM 16: CONTEXT REQUIRED (AC/FAN dung context) ----
    ("mo dieu hoa", "AC_ON", 1, None),
    ("bat may lanh", "AC_ON", 1, None),
    ("tat dieu hoa", "AC_OFF", 2, None),
    ("dong dieu hoa", "AC_OFF", 2, None),
    ("tat quat", "FAN_OFF", 7, None),
    # ---- NHOM 17: SET_TEMPERATURE - SO VUNG MIEN / STT BO SOT "muoi" ----
    ("đặt điều hòa hai ba độ", "SET_TEMPERATURE", 323, 23),
    ("đặt điều hòa hai tư độ", "SET_TEMPERATURE", 324, 24),
    ("đặt điều hòa hai lăm độ", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hòa hai sáu độ", "SET_TEMPERATURE", 326, 26),
    ("đặt điều hòa hai bảy độ", "SET_TEMPERATURE", 327, 27),
    ("đặt điều hòa hai tám độ", "SET_TEMPERATURE", 328, 28),
    ("đặt điều hòa hai chín độ", "SET_TEMPERATURE", 329, 29),
    ("đặt điều hòa hai ba", "SET_TEMPERATURE", 323, 23),
    ("đặt điều hòa hai tư", "SET_TEMPERATURE", 324, 24),
    ("đặt điều hòa hai lăm", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hòa hai nam độ", "SET_TEMPERATURE", 325, 25),
    ("đặt điều hoà hai lăm độ", "SET_TEMPERATURE", 325, 25),
    ("đặt máy lạnh hai lăm độ", "SET_TEMPERATURE", 325, 25),
    ("đặt máy lạnh hai mươi lăm", "SET_TEMPERATURE", 325, 25),
    ("đặt máy lạnh hai mươi ba", "SET_TEMPERATURE", 323, 23),
    ("cho điều hòa hai tư", "SET_TEMPERATURE", 324, 24),
    ("cho máy lạnh hai lăm", "SET_TEMPERATURE", 325, 25),
    # ---- NHOM 18: FAN - "quạt lên" (chi trang tu) ----
    ("quạt lên", "FAN_ON", 6, None),
    ("bật quạt lên", "FAN_ON", 6, None),
    ("mở quạt lên", "FAN_ON", 6, None),
    ("quạt", "UNKNOWN", None, None),
    ("quạt xuống", "UNKNOWN", None, None),
    ("cái quạt", "UNKNOWN", None, None),
    # ---- NHOM 19: TANG 3 - LOI STT/VUNG MIEN (gate thiet bi) ----
    ("bạc điều hòa", "AC_ON", 1, None),
    ("bạc máy lạnh", "AC_ON", 1, None),
    ("bạc quạt", "FAN_ON", 6, None),
    ("mở máy lạng", "AC_ON", 1, None),
    ("tắt máy lạng", "AC_OFF", 2, None),
    ("tắc điều hòa", "AC_OFF", 2, None),
    ("tắc quạt", "FAN_OFF", 7, None),
    ("quạc lên", "FAN_ON", 6, None),
    # ---- NHOM 19b: TANG 3 - PHU DINH (khong false positive) ----
    ("bạc", "UNKNOWN", None, None),
    ("máy lạng", "UNKNOWN", None, None),
    ("tắc đường", "UNKNOWN", None, None),
    ("quạc quạc", "UNKNOWN", None, None),
    # ---- NHOM 20: VUNG MIEN TONG HOP (vi du tu yeu cau) ----
    ("bặt điều hòa", "AC_ON", 1, None),
    ("tắt điều hoà đi", "AC_OFF", 2, None),
    ("tăng máy lạnh lên", "TEMP_UP", 4, None),
    ("giảm máy lạnh xuống", "TEMP_DOWN", 5, None),
    ("nhiệt độ lên", "TEMP_UP", 4, None),
    ("nhiệt độ xuống", "TEMP_DOWN", 5, None),
    ("bật sưởi kính", "AIR_DEFROST", 10, None),
    ("bật sưởi kính chắn gió", "AIR_DEFROST", 10, None),
    ("tắt quạt đi", "FAN_OFF", 7, None),
    ("đặt 25 độ", "SET_TEMPERATURE", 325, 25),
    # ---- NHOM 20b: SO KHONG CONTEXT -> UNKNOWN (an toan) ----
    ("hai ba", "UNKNOWN", None, None),
    ("hai lăm", "UNKNOWN", None, None),
    ("hai tư", "UNKNOWN", None, None),
]

GROUPS = [
    (1, "AC_ON", "AC_ON"),
    (2, "AC_OFF", "AC_OFF"),
    (3, "TEMP_UP", "TEMP_UP"),
    (4, "TEMP_DOWN", "TEMP_DOWN"),
    (5, "FAN_ON", "FAN_ON"),
    (6, "FAN_OFF", "FAN_OFF"),
    (7, "AIR_FACE", "AIR_FACE"),
    (8, "AIR_FOOT", "AIR_FOOT"),
    (9, "AIR_DEFROST", "AIR_DEFROST"),
    (7, "AIR_AUTO", "AIR_AUTO"),
    (10, "SET_TEMPERATURE", "SET_TEMPERATURE"),
    (11, "INVALID_TEMPERATURE", "INVALID_TEMPERATURE"),
    (12, "UNKNOWN", "UNKNOWN"),
    (13, "HELLO", "HELLO"),
    (13, "GOODBYE", "GOODBYE"),
    (14, "VOSK GARBAGE", "UNKNOWN"),
    (15, "NEGATION", "UNKNOWN"),
    (16, "AC/FAN CONTEXT", "AC_ON"),
    (17, "SET SỐ VÙNG MIỀN", "SET_TEMPERATURE"),
    (18, "FAN ADVERB", "FAN_ON"),
    (18, "FAN ADVERB SAFETY", "UNKNOWN"),
    (19, "TẦNG 3 LỖI STT", "AC_ON"),
    (19, "TẦNG 3 PHỦ ĐỊNH", "UNKNOWN"),
    (20, "VÙNG MIỀN TỔNG HỢP", "AC_ON"),
    (20, "SỐ KHÔNG CONTEXT", "UNKNOWN"),
]


def main() -> int:
    total = 0
    passed = 0
    failed = 0

    current_group: str | None = None
    for text, expected_intent, expected_code, expected_temp in CASES:
        group = next((name for _, name, intent in GROUPS if intent == expected_intent), "")
        if group != current_group:
            current_group = group
            print(f"\n=== NHOM {group} ===")

        result = classify(text)
        total += 1

        ok = (
            result.intent == expected_intent
            and result.command_code == expected_code
            and result.temperature == expected_temp
        )

        actual = (
            f"intent={result.intent}, code={result.command_code}, "
            f"temp={result.temperature}"
        )
        expected = (
            f"intent={expected_intent}, code={expected_code}, temp={expected_temp}"
        )

        if ok:
            passed += 1
            print(f"[PASS] input={text!r}")
            print(f"      expected: {expected}")
            print(f"      actual:   {actual}")
        else:
            failed += 1
            print(f"[FAIL] input={text!r}")
            print(f"      expected: {expected}")
            print(f"      actual:   {actual}")

    print("\n========================================")
    print(f"TOTAL: {total}")
    print(f"PASS : {passed}")
    print(f"FAIL : {failed}")
    print("========================================")

    return 1 if failed > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
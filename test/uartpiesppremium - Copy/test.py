from command_ai import classify

tests = [
    "đặt nhiệt độ 25 độ",
    "đặt nhiệt độ mười chín độ",
    "đặt nhiệt độ mười bảy độ",
    "đặt nhiệt độ ba mươi mốt độ",
    "đặt điều hoà ở 25 độ C",
    "đặt máy lạnh 26 độ",
    "đặt nhiệt độ mười tám độ C",
    "cho điều hoà 18 độ",
    "dừng quạt",
    "dừng",
    "hãy đặt nhiệt độ",
]

for x in tests:
    r = classify(x)
    print(x, "=>", r.intent, r.command_code, r.temperature, "|", r.response)
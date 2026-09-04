#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 03 - GPIO OUTPUT / RELAY
// Phu hop: ESP32-S3 N16R8 CH343P
// Muc dich: test 3 relay output don gian theo PinConfig.h (KHONG sua PinConfig)
//   GPIO4 = AC Relay
//   GPIO5 = Fan Relay
//   GPIO6 = Jetson/Pi Power Relay
//   GPIO7 KHONG thuoc TEST03 (danh cho TEST06 PWM/FET)
// Quy uoc: LOW = OFF, HIGH = ON (active HIGH) - KHONG dao polarity
// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9 - chi hien thi gon 7 dong
// Serial 115200 - hien thi debug chi tiet day du
//
// KHONG bat 12V, KHONG dieu khien tai cong suat
// KHONG test PWM tai day (xem TEST06)
// KHONG dieu khien Motor tai day (xem TEST07)
// Test CHAY 1 LAN roi dung sau FINAL RESULT - khong lap cycle vo han

// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;

// GPIO - GIU NGUYEN theo yeu cau (KHONG sua PinConfig.h, KHONG doi GPIO, KHONG doi polarity)
#define PIN_AC_RELAY        4
#define PIN_FAN_RELAY       5
#define PIN_PI_POWER_RELAY  6
// PIN_FAN_FET_PWM 7 KHONG thuoc TEST03 - danh cho TEST06

// Chi test 3 relay
const uint8_t kRelayPins[3] = { PIN_AC_RELAY, PIN_FAN_RELAY, PIN_PI_POWER_RELAY };
const char* kRelayLabels[3] = { "AC RELAY", "FAN RELAY", "JETSON RELAY" };
const char* kRelayShort[3]  = { "AC", "FAN", "JET" };

// Delay ngan de quan sat relay (500-1000ms), khong dung delay dai
#define DELAY_OFF_MS 600
#define DELAY_ON_MS  800

// Trang thai hien thi OLED
bool g_acOn = false;
bool g_fanOn = false;
bool g_jetOn = false;

// Ket qua test
bool g_bootAllOffPass = false;
bool g_resultAc = false;
bool g_resultFan = false;
bool g_resultJet = false;
bool g_overallPass = false;
bool g_testDone = false;

void oledShow(const char* status) {
    if (!oled_ok) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    // Toi da 7 dong - gon, khong tran 128x64
    display.println("RELAY TEST");
    display.println("");
    display.print("AC    : ");
    display.println(g_acOn ? "ON" : "OFF");
    display.print("FAN   : ");
    display.println(g_fanOn ? "ON" : "OFF");
    display.print("JET   : ");
    display.println(g_jetOn ? "ON" : "OFF");
    display.println("");
    display.print("STATUS: ");
    display.println(status);
    display.display();
}

void updateGlobalStateFromPins() {
    g_acOn  = digitalRead(PIN_AC_RELAY) == HIGH;
    g_fanOn = digitalRead(PIN_FAN_RELAY) == HIGH;
    g_jetOn = digitalRead(PIN_PI_POWER_RELAY) == HIGH;
}

void setRelayLevel(uint8_t pin, int level) {
    digitalWrite(pin, level);
    // cap nhat trang thai OLED ngay sau khi ghi
    if (pin == PIN_AC_RELAY) g_acOn = (level == HIGH);
    else if (pin == PIN_FAN_RELAY) g_fanOn = (level == HIGH);
    else if (pin == PIN_PI_POWER_RELAY) g_jetOn = (level == HIGH);
}

void setAllLowSafe() {
    digitalWrite(PIN_AC_RELAY, LOW);
    digitalWrite(PIN_FAN_RELAY, LOW);
    digitalWrite(PIN_PI_POWER_RELAY, LOW);
    g_acOn = false;
    g_fanOn = false;
    g_jetOn = false;
}

void printHeader() {
    Serial.println("================================");
    Serial.println("GPIO OUTPUT / RELAY TEST");
    Serial.println("================================");
    Serial.println();
}

void printPinMapping() {
    Serial.println("AC RELAY     GPIO4");
    Serial.println("FAN RELAY    GPIO5");
    Serial.println("JETSON RELAY GPIO6");
    Serial.println();
}

void printBootState() {
    Serial.println("BOOT STATE");
    Serial.printf("GPIO4: %s\n", digitalRead(PIN_AC_RELAY) ? "ON" : "OFF");
    Serial.printf("GPIO5: %s\n", digitalRead(PIN_FAN_RELAY) ? "ON" : "OFF");
    Serial.printf("GPIO6: %s\n", digitalRead(PIN_PI_POWER_RELAY) ? "ON" : "OFF");
    Serial.println();
}

// Test 1 relay theo chu trinh OFF -> ON -> OFF, verify bang digitalRead sau moi digitalWrite
// Tra ve true neu ca 3 buoc dung level mong doi (LOW/HIGH/LOW)
bool runSingleRelayTest(uint8_t pin, const char* label, const char* statusLabel) {
    Serial.println("--------------------------------");
    Serial.printf("TEST GPIO%d - %s\n", pin, label);
    Serial.println("--------------------------------");

    bool pass = true;

    // Buoc 1: OFF
    setRelayLevel(pin, LOW);
    oledShow(statusLabel);
    delay(DELAY_OFF_MS);
    {
        int lvl = digitalRead(pin);
        const char* lvlStr = (lvl == HIGH) ? "HIGH" : "LOW";
        Serial.println("STATE: OFF");
        Serial.printf("GPIO LEVEL: %s\n", lvlStr);
        Serial.println();
        // log chi tiet theo yeu cau: ten relay, GPIO, command/state, actual level
        Serial.printf("[%s] GPIO%d | CMD: OFF | LEVEL: %s | %s\n", label, pin, lvlStr, (lvl == LOW) ? "OK" : "MISMATCH");
        if (lvl != LOW) pass = false;
    }

    // Buoc 2: ON
    setRelayLevel(pin, HIGH);
    oledShow(statusLabel);
    delay(DELAY_ON_MS);
    {
        int lvl = digitalRead(pin);
        const char* lvlStr = (lvl == HIGH) ? "HIGH" : "LOW";
        Serial.println("STATE: ON");
        Serial.printf("GPIO LEVEL: %s\n", lvlStr);
        Serial.println();
        Serial.printf("[%s] GPIO%d | CMD: ON  | LEVEL: %s | %s\n", label, pin, lvlStr, (lvl == HIGH) ? "OK" : "MISMATCH");
        if (lvl != HIGH) pass = false;
    }

    // Buoc 3: OFF lai (ve safe state)
    setRelayLevel(pin, LOW);
    oledShow(statusLabel);
    delay(DELAY_OFF_MS);
    {
        int lvl = digitalRead(pin);
        const char* lvlStr = (lvl == HIGH) ? "HIGH" : "LOW";
        Serial.println("STATE: OFF");
        Serial.printf("GPIO LEVEL: %s\n", lvlStr);
        Serial.println();
        Serial.printf("[%s] GPIO%d | CMD: OFF | LEVEL: %s | %s\n", label, pin, lvlStr, (lvl == LOW) ? "OK" : "MISMATCH");
        if (lvl != LOW) pass = false;
    }

    Serial.printf("RESULT: %s\n", pass ? "PASS" : "FAIL");
    Serial.println();
    return pass;
}

void runAllTestsOnce() {
    // Giai doan TEST AC
    g_resultAc = runSingleRelayTest(PIN_AC_RELAY, "AC RELAY", "TEST AC");
    // Dam bao ve safe OFF truoc relay tiep theo
    setAllLowSafe();

    // Giai doan TEST FAN
    g_resultFan = runSingleRelayTest(PIN_FAN_RELAY, "FAN RELAY", "TEST FAN");
    setAllLowSafe();

    // Giai doan TEST JETSON
    g_resultJet = runSingleRelayTest(PIN_PI_POWER_RELAY, "JETSON RELAY", "TEST JET");
    setAllLowSafe();

    // Final summary
    g_overallPass = g_bootAllOffPass && g_resultAc && g_resultFan && g_resultJet;

    Serial.println("FINAL RESULT");
    Serial.printf("GPIO4 AC RELAY      : %s\n", g_resultAc ? "PASS" : "FAIL");
    Serial.printf("GPIO5 FAN RELAY     : %s\n", g_resultFan ? "PASS" : "FAIL");
    Serial.printf("GPIO6 JETSON RELAY  : %s\n", g_resultJet ? "PASS" : "FAIL");
    Serial.printf("BOOT ALL OFF        : %s\n", g_bootAllOffPass ? "PASS" : "FAIL");
    Serial.printf("OVERALL             : %s\n", g_overallPass ? "PASS" : "FAIL");
    Serial.println();

    // OLED cuoi cung hien thi PASS/FAIL tong
    oledShow(g_overallPass ? "PASS" : "FAIL");
    g_testDone = true;
}

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial && millis() < 3000) {}

    printHeader();
    printPinMapping();

    // Init 3 relay pins ve safe OFF (LOW = OFF) - KHONG dao polarity
    pinMode(PIN_AC_RELAY, OUTPUT);
    pinMode(PIN_FAN_RELAY, OUTPUT);
    pinMode(PIN_PI_POWER_RELAY, OUTPUT);
    setAllLowSafe();
    delay(50);

    // Boot state check - ca 3 phai OFF khi khoi dong
    bool bootAcOff  = digitalRead(PIN_AC_RELAY) == LOW;
    bool bootFanOff = digitalRead(PIN_FAN_RELAY) == LOW;
    bool bootJetOff = digitalRead(PIN_PI_POWER_RELAY) == LOW;
    g_bootAllOffPass = bootAcOff && bootFanOff && bootJetOff;

    printBootState();

    // OLED init
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        oled_ok = false;
        Serial.println("[OLED] INIT FAIL");
    } else {
        oled_ok = true;
        oledShow("READY");
        Serial.println("[OLED] INIT PASS - 128x64 READY");
        Serial.println();
    }

    // Ghi chu an toan (khong thuc thi nguon 12V)
    Serial.println("[SAFETY] Relays switch AC/DC - no 12V load in this test");
    Serial.println("[SAFETY] LOW=OFF HIGH=ON - polarity preserved");
    Serial.println();

    // Chay test 1 lan duy nhat
    runAllTestsOnce();
}

void loop() {
    // Test da chay xong trong setup() - chi duy tri OLED hien thi FINAL PASS/FAIL
    // Khong lap cycle vo han theo yeu cau
    // Giu OLED refresh nhe neu can (khong can cap nhat lien tuc)
    if (g_testDone) {
        // duy tri hien thi cuoi cung, khong lam gi them
        delay(1000);
        return;
    }
}

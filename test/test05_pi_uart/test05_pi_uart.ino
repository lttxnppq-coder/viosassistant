#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 05 - Jetson UART ASCII TEST (ESP32 <-> Jetson)
// UART Jetson: TX GPIO17 -> Jetson RX, RX GPIO18 <- Jetson TX, 9600 8N1 ASCII + CR
// USB debug Serial: 115200 for Serial Monitor logs
// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9 — MONITOR ONLY, no overflow
//
// KHONG dung 115200 cho UART Jetson, KHONG binary, KHONG AA/LEN/CRC/55
// KHONG dieu khien motor/relay/fan

// OLED
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;

// UART Jetson — tach khoi USB debug Serial
#define PIN_UART_TX 17 // ESP32 TX -> Jetson RX
#define PIN_UART_RX 18 // ESP32 RX <- Jetson TX
#define UART_BAUD 9600

// State for test
uint32_t rx_count = 0;
uint32_t tx_count = 0;
String rx_buffer = "";
String last_raw = "";
String last_cmd_str = "";
String last_result = "WAITING";
unsigned long last_status_ms = 0;

// Helper: map command code to description (per arduino_simulator.ino)
const char* cmdToStr(int code) {
  if (code == 1) return "AC ON";
  if (code == 2) return "AC OFF";
  if (code == 4) return "TEMP +2";
  if (code == 5) return "TEMP -2";
  if (code == 6) return "FAN ON";
  if (code == 7) return "FAN OFF";
  if (code == 8) return "FACE";
  if (code == 9) return "FOOT";
  if (code == 10) return "FACE+FOOT";
  if (code >= 101 && code <= 105) return "FAN LEVEL";
  if (code >= 324 && code <= 332) return "TEMP 24-32";
  return "UNKNOWN";
}

bool isValidCmd(int code) {
  if (code == 1 || code == 2 || code == 4 || code == 5 || code == 6 || code == 7 || code == 8 || code == 9 || code == 10) return true;
  if (code >= 101 && code <= 105) return true;
  if (code >= 324 && code <= 332) return true;
  return false;
}

void updateOled() {
  if (!oled_ok) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("UART TEST");
  display.println("9600 ASCII");
  display.println("----------------");
  char buf[32];
  snprintf(buf, sizeof(buf), "RX:%lu TX:%lu", (unsigned long)rx_count, (unsigned long)tx_count);
  display.println(buf);
  display.print("LAST:");
  if (last_raw.length() == 0) display.println(" N/A");
  else {
    // Giu trong 21 ky tu: "LAST:101" = 8
    display.println(last_raw);
  }
  display.print("STATUS:");
  display.println(last_result);
  display.display();
}

void setup() {
  // USB debug Serial — 115200 rieng, khong nham voi UART Jetson 9600
  Serial.begin(115200);
  delay(100);
  while (!Serial && millis() < 3000) {}

  Serial.println();
  Serial.println("================================");
  Serial.println("UART ASCII TEST");
  Serial.println("================================");
  Serial.println("UART: Serial1");
  Serial.printf("TX: GPIO%d\n", PIN_UART_TX);
  Serial.printf("RX: GPIO%d\n", PIN_UART_RX);
  Serial.printf("BAUD: %d\n", UART_BAUD);
  Serial.println("FORMAT: ASCII");
  Serial.println("TERM: CR");
  Serial.println("================================");
  Serial.println();

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oled_ok = false;
    Serial.println("[OLED] INIT FAIL");
  } else {
    oled_ok = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("UART TEST");
    display.println("9600 ASCII");
    display.println("----------------");
    display.println("INIT WAITING");
    display.display();
    Serial.println("[OLED] INIT PASS");
  }

  // Jetson UART — 9600 ASCII + CR, tach khoi USB debug
  Serial1.begin(UART_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
  Serial.println("[UART] Serial1 9600 ASCII init OK (17 TX -> Jetson RX, 18 RX <- Jetson TX)");

  rx_buffer.reserve(32);
  last_result = "WAITING";
  updateOled();
}

void loop() {
  // Nhan ASCII tu Jetson, ket thuc bang '\r'
  while (Serial1.available() > 0) {
    char c = (char)Serial1.read();
    if (c == '\r') {
      // Hoan thanh 1 command
      String raw = rx_buffer;
      rx_buffer = "";
      if (raw.length() == 0) {
        // Ignore empty
        continue;
      }
      rx_count++;
      last_raw = raw;
      int code = raw.toInt();

      Serial.println();
      Serial.println("[UART RX]");
      Serial.print("RAW: ");
      Serial.println(raw);

      const char* cmdStr = cmdToStr(code);
      bool valid = isValidCmd(code);

      if (valid) {
        last_cmd_str = cmdStr;
        last_result = "PASS";
        Serial.print("CMD: ");
        Serial.println(cmdStr);
        Serial.print("CODE: ");
        Serial.println(code);
        if (code >= 101 && code <= 105) {
          Serial.printf("DETAIL: FAN LEVEL %d\n", code - 100);
        } else if (code >= 324 && code <= 332) {
          Serial.printf("DETAIL: TEMP %dC\n", code - 300);
        }
        Serial.println("RESULT: PASS");
      } else {
        last_result = "FAIL";
        Serial.print("CMD: ");
        Serial.println(cmdStr);
        Serial.println("RESULT: INVALID");
      }

      // Cap nhat OLED: RX, LAST, STATUS
      updateOled();

      // Response: hien tai response format chua xac dinh ro trong production
      // De tranh tu phat minh protocol, test nay lam RX-only (khong gui TX tu dong)
      // Neu can TX, co the echo lai raw + CRLF de Jetson verify loopback:
      // Serial1.print(raw); Serial1.print("\r\n"); tx_count++; // KHONG bat buoc
      // O day giu TX count = 0 va bao NOT TESTED neu chua gui
    } else if (c == '\n') {
      // Ignore LF
    } else {
      rx_buffer += c;
      // Simple overflow guard
      if (rx_buffer.length() > 32) rx_buffer = "";
    }
  }

  // OLED refresh nhe (khong block)
  if (millis() - last_status_ms > 300) {
    last_status_ms = millis();
    updateOled();
  }

  // Safety: khong dieu khien motor/relay/fan trong test nay
}

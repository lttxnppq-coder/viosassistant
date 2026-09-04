#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// TEST 05 JETSON UART — REAL UART TEST ONLY: JETSON -> ESP32 -> ACK -> OLED
// UART: 9600 ASCII, command terminated by '\r'
// Jetson TX -> ESP32 RX GPIO18, ESP32 TX GPIO17 -> Jetson RX
// USB debug Serial: 115200
// OLED SSD1306 128x64 I2C 0x3C SDA8 SCL9 — single page, no overflow
// KHONG binary, KHONG AA/LEN/CRC/55, KHONG production protocol

#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool oled_ok = false;

#define PIN_UART_TX 17
#define PIN_UART_RX 18
#define UART_BAUD 9600

// State — real counters, no fake
uint32_t rx_count = 0;
uint32_t tx_count = 0;
String rx_buffer = "";
String last_rx = "";
String ack_status = "NONE"; // SENT / NONE
String oled_status = "IDLE"; // RECEIVED / IDLE
unsigned long last_oled_ms = 0;

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
  display.printf("BAUD: %d\n", UART_BAUD);
  display.println("----------------");
  char buf[32];
  snprintf(buf, sizeof(buf), "RX:%lu TX:%lu", (unsigned long)rx_count, (unsigned long)tx_count);
  display.println(buf);
  display.print("LAST RX:");
  if (last_rx.length() == 0) display.println(" N/A");
  else display.println(last_rx);
  display.print("STATUS:");
  display.println(oled_status);
  display.print("ACK:");
  display.println(ack_status);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  while (!Serial && millis() < 3000) {}

  Serial.println();
  Serial.println("=== UART TEST 9600 ===");
  Serial.println("READY");
  Serial.println("UART: Serial1");
  Serial.printf("TX: GPIO%d\n", PIN_UART_TX);
  Serial.printf("RX: GPIO%d\n", PIN_UART_RX);
  Serial.printf("BAUD: %d\n", UART_BAUD);
  Serial.println("FORMAT: ASCII");
  Serial.println("TERM: CR");
  Serial.println("======================");

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
    display.println("BAUD: 9600");
    display.println("----------------");
    display.println("RX:0 TX:0");
    display.println("LAST RX: N/A");
    display.println("STATUS: IDLE");
    display.println("ACK: NONE");
    display.display();
    Serial.println("[OLED] INIT PASS");
  }

  Serial1.begin(UART_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
  Serial.println("[UART] Serial1 9600 init OK (17 TX -> Jetson RX, 18 RX <- Jetson TX)");

  rx_buffer.reserve(32);
  oled_status = "IDLE";
  ack_status = "NONE";
  updateOled();
}

void loop() {
  // Jetson -> ESP32 : ASCII + '\r' (e.g. "101\r")
  while (Serial1.available() > 0) {
    char c = (char)Serial1.read();
    if (c == '\r') {
      String raw = rx_buffer;
      rx_buffer = "";
      if (raw.length() == 0) continue;

      // 1 RX frame -> 1 RX count
      rx_count++;
      last_rx = raw;
      oled_status = "RECEIVED";

      Serial.println();
      Serial.print("[UART RX] ");
      Serial.println(raw);

      // Validate (optional) but still ACK even if invalid? Spec: ACK for received
      // For this minimal test, ACK every received raw
      String ack = "ACK " + raw + "\r";
      Serial1.print(ack);
      tx_count++;
      ack_status = "SENT";

      Serial.print("[UART TX] ");
      Serial.print(ack); // includes \r, will show as new line in monitor
      // For Serial Monitor visibility without \r confusion, also print without \r
      // Already printed above with \r

      updateOled();
    } else if (c == '\n') {
      // ignore LF
    } else {
      rx_buffer += c;
      if (rx_buffer.length() > 32) rx_buffer = ""; // overflow guard
    }
  }

  // OLED refresh non-blocking
  if (millis() - last_oled_ms > 300) {
    last_oled_ms = millis();
    // If no RX for a while, show IDLE
    // Keep RECEIVED until next RX, or timeout to IDLE after 2s
    static unsigned long last_rx_ms = 0;
    if (rx_count > 0 && oled_status == "RECEIVED") {
      if (last_rx.length() > 0) {
        // Keep last_rx_ms updated on each RX
        // Use static to track
      }
    }
    updateOled();
  }
}

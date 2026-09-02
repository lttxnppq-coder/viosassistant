#include "oled_display.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Cau hinh OLED (da xac nhan): GM009605V4.3, SSD1306, 128x64, I2C
// ---------------------------------------------------------------------------

#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_SDA     41
#define OLED_SCL     42
#define OLED_ADDR_A  0x3C
#define OLED_ADDR_B  0x3D
#define PHASE_MS     400  // thoi gian hien thi moi trang thai (non-blocking)

static Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

static bool oled_ok = false;

static const char *status_text = "READY";
static int status_cmd = -2;  // -2 = "--" (chua co lenh), -1 = "??" (loi), >=0 = code
static int phase = 0;        // 0 = static (READY/ERROR), 1 = RECEIVED, 2 = EXECUTING, 3 = DONE
static unsigned long phase_start = 0;
static bool auto_reset = false;  // dang hien thi DONE/ERROR -> het PHASE_MS tu quay ve READY

// ---------------------------------------------------------------------------
// Render 128x64: SMART AC / STATUS / <STATUS LON> / CMD: xxx
// ---------------------------------------------------------------------------

static void oled_render() {
    if (!oled_ok) return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(16, 0);
    display.print(F("SMART AC"));

    display.setTextSize(1);
    display.setCursor(46, 18);
    display.print(F("STATUS"));

    display.setTextSize(2);
    display.setCursor(10, 32);
    display.print(status_text);

    display.setTextSize(1);
    display.setCursor(8, 54);
    display.print(F("CMD: "));
    if (status_cmd == -2) {
        display.print(F("--"));
    } else if (status_cmd == -1) {
        display.print(F("??"));
    } else {
        display.print(status_cmd);
    }

    display.display();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool oled_init() {
    Wire.begin(OLED_SDA, OLED_SCL);
    delay(10);

    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_A)) {
        oled_ok = true;
    } else if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_B)) {
        oled_ok = true;
    }

    if (!oled_ok) {
        Serial.println("OLED NOT FOUND");
        return false;
    }

    display.clearDisplay();
    display.display();
    return true;
}

void oled_show_ready() {
    if (!oled_ok) return;
    status_text = "READY";
    status_cmd = -2;
    phase = 0;
    auto_reset = false;
    oled_render();
}

void oled_show_received(int cmd) {
    if (!oled_ok) return;
    status_text = "RECEIVED";
    status_cmd = cmd;
    phase = 1;
    phase_start = millis();
    oled_render();
}

void oled_show_executing(int cmd) {
    if (!oled_ok) return;
    status_text = "EXECUTING";
    status_cmd = cmd;
    phase = 2;
    phase_start = millis();
    oled_render();
}

void oled_show_done(int cmd) {
    if (!oled_ok) return;
    status_text = "DONE";
    status_cmd = cmd;
    phase = 3;
    phase_start = millis();
    auto_reset = true;
    oled_render();
}

void oled_show_error(int cmd) {
    if (!oled_ok) return;
    status_text = "ERROR";
    status_cmd = (cmd >= 0) ? cmd : -1;
    phase = 0;
    phase_start = millis();
    auto_reset = true;
    oled_render();
}

// ---------------------------------------------------------------------------
// Non-blocking: RECEIVED -> (400ms) EXECUTING -> (400ms) DONE -> (400ms) READY.
// ERROR -> (400ms) READY. Lenh moi den se reset phase qua oled_show_received().
// ---------------------------------------------------------------------------

void oled_update() {
    if (!oled_ok) return;

    unsigned long elapsed = (unsigned long)(millis() - phase_start);

    if (phase == 1 && elapsed >= PHASE_MS) {
        status_text = "EXECUTING";
        phase = 2;
        phase_start = millis();
        oled_render();
    } else if (phase == 2 && elapsed >= PHASE_MS) {
        status_text = "DONE";
        phase = 3;
        phase_start = millis();
        oled_render();
    } else if (phase == 3 && elapsed >= PHASE_MS) {
        // DONE xong -> tu quay ve READY
        oled_show_ready();
    } else if (phase == 0 && auto_reset && elapsed >= PHASE_MS) {
        // ERROR xong -> tu quay ve READY
        oled_show_ready();
    }
}

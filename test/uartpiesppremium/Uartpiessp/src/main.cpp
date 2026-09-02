#include <Arduino.h>

#include "oled_display.h"

// Protocol thong nhat (Python -> ESP32) — PHASE 1: chi AC/UART control:
//     CMD:AC_ON\n / CMD:AC_OFF\n / CMD:TEMP_UP\n / CMD:TEMP_DOWN\n
//     CMD:FAN_ON\n / CMD:FAN_OFF\n / CMD:AIR_FACE\n / CMD:AIR_FOOT\n / CMD:AIR_DEFROST\n
//     CMD:AIR_AUTO\n / CMD:SET_TEMP:<18..30>\n
// ESP32 -> Python:
//     RESP:OK:<CMD>\n                        (AC commands)
//     RESP:OK:SET_TEMP:<temp>\n
//     RESP:ERROR:INVALID_TEMPERATURE\n
//     RESP:ERROR:UNKNOWN_COMMAND\n
//
// Robot protocol (HELLO/FORWARD/BACKWARD/LEFT/RIGHT/STOP) se build o phase sau,
// KHONG giu backward compatibility trong firmware Phase 1.
//
// Serial Monitor debug (observability) — KHONG thuoc protocol:
//     [SYSTEM][<ms> ms] Boot
//     [UART_RX][<ms> ms] len=<N> raw=<line>   (truoc trim -> phat hien \r\n, cat line)
//     [PARSE][<ms> ms] Command=<cmd>
//     [ACTION][<ms> ms] Handler=<cmd> (ACK-only prototype)
//     [UART_TX][<ms> ms] RESP:...
//     [ERROR][<ms> ms] <reason>
// Moi debug line bat dau bang '[', chi protocol line bat dau bang "RESP:".
// Python parser chi doc line startswith("RESP:") va banner "ESP32 READY".
//
// "ESP32 READY" chi la BOOT HANDSHAKE, khong phai response cua command.
// Python convert 318..330 -> CMD:SET_TEMP:<n>; firmware KHONG nhan dang CMD:318.
// KHONG thay doi GPIO/motor trong file nay (chi protocol/decode/ACK + debug log).

void debug_log(const char *tag, const String &msg) {
    Serial.print('[');
    Serial.print(tag);
    Serial.print("][");
    Serial.print(millis());
    Serial.print(" ms] ");
    Serial.println(msg);
}

void setup() {
    Serial.begin(115200);

    debug_log("SYSTEM", "Boot");
    debug_log("SYSTEM", "Serial initialized");

    delay(1000);

    Serial.println("ESP32 READY");

    // OLED status display: neu khong tim thay OLED thi firmware van chay binh thuong.
    if (oled_init()) {
        oled_show_ready();
    }
}

// Map lenh -> CODE so (dung mapping hien tai cua project, xem command_ai.py):
//     AC_ON=1 AC_OFF=2 TEMP_UP=4 TEMP_DOWN=5 FAN_ON=6 FAN_OFF=7
//     AIR_FACE=8 AIR_FOOT=9 AIR_DEFROST=10 AIR_AUTO=11
//     SET_TEMP:<18..30> -> 300 + nhiet do (323..330)
// Tra -1 neu lenh khong hop le (OLED hien "CMD: ??").
static int command_to_code(const String &command) {
    if (command == "AC_ON") return 1;
    if (command == "AC_OFF") return 2;
    if (command == "TEMP_UP") return 4;
    if (command == "TEMP_DOWN") return 5;
    if (command == "FAN_ON") return 6;
    if (command == "FAN_OFF") return 7;
    if (command == "AIR_FACE") return 8;
    if (command == "AIR_FOOT") return 9;
    if (command == "AIR_DEFROST") return 10;
    if (command == "AIR_AUTO") return 11;

    if (command.startsWith("SET_TEMP:")) {
        String tempStr = command.substring(9);
        tempStr.trim();
        if (tempStr.length() == 0) return -1;
        for (unsigned int i = 0; i < tempStr.length(); i++) {
            if (!isdigit((unsigned char)tempStr[i])) return -1;
        }
        int temp = tempStr.toInt();
        if (temp < 18 || temp > 30) return -1;
        return 300 + temp;
    }

    return -1;
}

void send_resp_ac(const String &command) {
    debug_log("ACTION", "Handler=" + command + " (ACK-only prototype)");
    debug_log("UART_TX", "RESP:OK:" + command);
    Serial.print("RESP:OK:");
    Serial.println(command);
}

void handle_set_temp(const String &command) {
    String tempStr = command.substring(9);
    tempStr.trim();

    debug_log("PARSE", "SET_TEMP value=" + tempStr);

    bool isDigits = tempStr.length() > 0;
    for (unsigned int i = 0; isDigits && i < tempStr.length(); i++) {
        if (!isdigit((unsigned char)tempStr[i])) {
            isDigits = false;
        }
    }

    if (!isDigits) {
        debug_log("ERROR", "Invalid temperature value=" + tempStr);
        debug_log("UART_TX", "RESP:ERROR:INVALID_TEMPERATURE");
        Serial.println("RESP:ERROR:INVALID_TEMPERATURE");
        return;
    }

    int temp = tempStr.toInt();
    if (temp < 18 || temp > 30) {
        debug_log("ERROR", "Temperature out of range=" + tempStr);
        debug_log("UART_TX", "RESP:ERROR:INVALID_TEMPERATURE");
        Serial.println("RESP:ERROR:INVALID_TEMPERATURE");
        return;
    }

    debug_log("ACTION", "Handler=" + command + " (ACK-only prototype)");
    debug_log("UART_TX", "RESP:OK:SET_TEMP:" + String(temp));
    Serial.print("RESP:OK:SET_TEMP:");
    Serial.println(temp);
}

void loop() {

    if (Serial.available()) {

        String line = Serial.readStringUntil('\n');

        debug_log("UART_RX", "len=" + String(line.length()) + " raw=" + line);

        line.trim();

        if (line.length() == 0) {
            debug_log("ERROR", "Empty command");
            return;
        }

        if (!line.startsWith("CMD:")) {
            debug_log("ERROR", "Missing CMD: prefix");
            debug_log("UART_TX", "RESP:ERROR:UNKNOWN_COMMAND");
            Serial.println("RESP:ERROR:UNKNOWN_COMMAND");
            return;
        }

        String command = line.substring(4);
        command.trim();

        debug_log("PARSE", "Command=" + command);

        // OLED status: RECEIVED (lenh hop le) / ERROR (lenh khong hop le).
        int oled_code = command_to_code(command);
        if (oled_code >= 0) {
            oled_show_received(oled_code);
        } else {
            oled_show_error(-1);
        }

        if (command == "AC_ON") {
            send_resp_ac("AC_ON");
        }
        else if (command == "AC_OFF") {
            send_resp_ac("AC_OFF");
        }
        else if (command == "TEMP_UP") {
            send_resp_ac("TEMP_UP");
        }
        else if (command == "TEMP_DOWN") {
            send_resp_ac("TEMP_DOWN");
        }
        else if (command == "FAN_ON") {
            send_resp_ac("FAN_ON");
        }
        else if (command == "FAN_OFF") {
            send_resp_ac("FAN_OFF");
        }
        else if (command == "AIR_FACE") {
            send_resp_ac("AIR_FACE");
        }
        else if (command == "AIR_FOOT") {
            send_resp_ac("AIR_FOOT");
        }
        else if (command == "AIR_DEFROST") {
            send_resp_ac("AIR_DEFROST");
        }
        else if (command == "AIR_AUTO") {
            send_resp_ac("AIR_AUTO");
        }
        else if (command.startsWith("SET_TEMP:")) {
            handle_set_temp(command);
        }
        else {
            debug_log("ERROR", "Unknown command=" + command);
            debug_log("UART_TX", "RESP:ERROR:UNKNOWN_COMMAND");
            Serial.println("RESP:ERROR:UNKNOWN_COMMAND");
        }
    }

    // OLED non-blocking: RECEIVED -> EXECUTING -> DONE (millis, khong delay).
    oled_update();
}
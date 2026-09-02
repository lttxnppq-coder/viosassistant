#include <Arduino.h>

// Protocol thong nhat (Python -> ESP32):
//     CMD:HELLO\n / CMD:FORWARD\n / CMD:BACKWARD\n / CMD:LEFT\n / CMD:RIGHT\n / CMD:STOP\n
// ESP32 -> Python:
//     RESP:OK:<CMD>:<message tieng Viet>\n
//     RESP:ERROR:UNKNOWN_COMMAND\n
//
// "ESP32 READY" chi la BOOT HANDSHAKE, khong phai response cua command.
// KHONG thay doi GPIO/motor trong file nay (chi prototype UART command/response).

void setup() {
    Serial.begin(115200);

    delay(1000);

    Serial.println("ESP32 READY");
}

void send_resp_ok(const char *command, const char *message) {
    Serial.print("RESP:OK:");
    Serial.print(command);
    Serial.print(":");
    Serial.println(message);
}

void loop() {

    if (Serial.available()) {

        String line = Serial.readStringUntil('\n');
        line.trim();

        if (line.length() == 0) {
            return;
        }

        if (!line.startsWith("CMD:")) {
            Serial.println("RESP:ERROR:UNKNOWN_COMMAND");
            return;
        }

        String command = line.substring(4);
        command.trim();

        if (command == "HELLO") {
            send_resp_ok("HELLO", "Xin chào, tôi đang hoạt động.");
        }
        else if (command == "FORWARD") {
            send_resp_ok("FORWARD", "Đã tiến lên.");
        }
        else if (command == "BACKWARD") {
            send_resp_ok("BACKWARD", "Đã lùi lại.");
        }
        else if (command == "LEFT") {
            send_resp_ok("LEFT", "Đã rẽ trái.");
        }
        else if (command == "RIGHT") {
            send_resp_ok("RIGHT", "Đã rẽ phải.");
        }
        else if (command == "STOP") {
            send_resp_ok("STOP", "Đã dừng lại.");
        }
        else {
            Serial.println("RESP:ERROR:UNKNOWN_COMMAND");
        }
    }
}

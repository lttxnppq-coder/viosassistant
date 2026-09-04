// arduino_simulator.ino — ORIGINAL VERSION (brother's simulator)
// Do NOT adapt to ESP32 binary protocol (115200/CRC)
// Serial 9600, 6 ASCII lines, delay 100ms, commands terminated by '\r'

int engine = 1;
int temperature = 24;
int AC = 0;
int wind_value = 0;
int last_wind_level = 1;
int last_mode = 0;
int door = 1;

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(9600);
  inputString.reserve(16);
}

void loop() {
  // Receive Python commands as ASCII terminated by '\r'
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') {
      stringComplete = true;
      break;
    } else if (c == '\n') {
      // ignore LF
    } else {
      inputString += c;
    }
  }

  if (stringComplete) {
    int code = inputString.toInt();
    inputString = "";
    stringComplete = false;

    if (code == 1) {
      AC = 1;
    } else if (code == 2) {
      AC = 0;
    } else if (code == 4) {
      temperature += 2;
      if (temperature > 32) temperature = 32;
    } else if (code == 5) {
      temperature -= 2;
      if (temperature < 24) temperature = 24;
    } else if (code == 6) {
      // turn fan ON: restore previous level
      wind_value = last_wind_level;
    } else if (code == 7) {
      // turn fan OFF: save current level
      if (wind_value != 0) last_wind_level = wind_value;
      wind_value = 0;
    } else if (code == 8) {
      last_mode = 0; // FACE
    } else if (code == 9) {
      last_mode = 1; // FOOT
    } else if (code == 10) {
      last_mode = 2; // FACE + FOOT
    } else if (code >= 101 && code <= 105) {
      wind_value = code - 100; // 1..5
      last_wind_level = wind_value;
    } else if (code >= 324 && code <= 332) {
      temperature = code - 300; // 24..32
    } else {
      // Unknown commands: ignore
    }
  }

  // Periodically send 6 ASCII lines in order: engine, temperature, AC, wind_value, last_mode, door
  Serial.println(engine);
  Serial.println(temperature);
  Serial.println(AC);
  Serial.println(wind_value);
  Serial.println(last_mode);
  Serial.println(door);

  delay(100);
}

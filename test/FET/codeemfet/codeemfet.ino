// ==========================================
// TEST MOSFET BLOWER - ARDUINO MEGA 2560
// SI -> PWM PIN 9
// ==========================================

#define SI_PIN 46


void setup()
{
  pinMode(SI_PIN, OUTPUT);

  // Ban đầu tắt PWM
  analogWrite(SI_PIN, 255);

  Serial.begin(9600);

  Serial.println("================================");
  Serial.println(" BLOWER MOSFET TEST");
  Serial.println(" Arduino Mega 2560");
  Serial.println(" SI -> PWM PIN 9");
  Serial.println("================================");
  Serial.println("Nhap:");
  Serial.println("0 = OFF");
  Serial.println("1 = LOW");
  Serial.println("2 = LEVEL 2");
  Serial.println("3 = LEVEL 3");
  Serial.println("4 = LEVEL 4");
  Serial.println("5 = MAX");
}


// MOSFET BLOWER ACTIVE-LOW
void setFanLevel(int level)
{
  int pwm;

  switch (level)
  {
    case 0:
      // OFF
      pwm = 255;
      break;

    case 1:
      // LOW
      pwm = 255;
      break;

    case 2:
      pwm = 191;   // ~75%
      break;

    case 3:
      pwm = 128;   // ~50%
      break;

    case 4:
      pwm = 64;    // ~25%
      break;

    case 5:
      // MAX
      pwm = 0;
      break;

    default:
      Serial.println("Chi nhap 0 - 5");
      return;
  }

  analogWrite(SI_PIN, pwm);

  Serial.print("Cap quat: ");
  Serial.print(level);

  Serial.print(" | PWM PIN 9: ");
  Serial.println(pwm);
}


void loop()
{
  if (Serial.available())
  {
    int level = Serial.parseInt();

    setFanLevel(level);

    while (Serial.available())
    {
      Serial.read();
    }
  }
}
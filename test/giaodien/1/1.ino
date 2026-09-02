#include <Arduino.h>

///*
int engine = 1;
int temperature = 24;
int door = 1;
int AC = 1;
int IG = 0;
int wind_value = 1;
int last_mode = 8;
//*/
String DATA = "";
void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT); 
}

void loop() {
 while(1){
    //*
    Serial.println(IG); 
    Serial.println(temperature);
    Serial.println(AC); 
    Serial.println(wind_value);
    Serial.println(last_mode); 
    Serial.println(door);
    delay(100); 
    //*/

    DATA = Serial.readStringUntil('\r');
    if (DATA == "1")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "2")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "3")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(300);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "4")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(400);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "5")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "6")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(600);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "7")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(700);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "8")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(800);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "9")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(900);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (DATA == "10")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
    }
 }

}

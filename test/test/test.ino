const int igPin = 18;    // Chân đọc tín hiệu IG
const int relayPin = 52; // Chân kích relay cấp nguồn 5V

void setup() {
  // Cấu hình chân 18 là INPUT_PULLUP để tránh tín hiệu bị nhiễu (trôi nổi)
  pinMode(igPin, INPUT_PULLUP); 
  pinMode(relayPin, OUTPUT);
}

void loop() {
  // Đọc trạng thái chân 18 liên tục
  int igState = digitalRead(igPin);
  
  // Nếu chân 18 bị kéo xuống LOW (IG ON) -> bật relay (chân 52 HIGH)
  // Nếu chân 18 thả nổi ở HIGH (IG OFF) -> tắt relay (chân 52 LOW)
  if (igState == LOW) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }
  
  // (Mẹo: Có thể viết gọn 1 dòng là: digitalWrite(relayPin, !digitalRead(igPin)); )
}
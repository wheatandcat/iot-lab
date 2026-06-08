/**
 * @brief リードスイッチ確認
 */
const int reedPin = D2;

void setup() {
  Serial.begin(115200);

  pinMode(reedPin, INPUT_PULLUP);

  Serial.println("Reed Test Start");
}

void loop() {
  int state = digitalRead(reedPin);

  if (state == LOW) {
    Serial.println("CLOSED");
  } else {
    Serial.println("OPEN");
  }

  delay(300);
}
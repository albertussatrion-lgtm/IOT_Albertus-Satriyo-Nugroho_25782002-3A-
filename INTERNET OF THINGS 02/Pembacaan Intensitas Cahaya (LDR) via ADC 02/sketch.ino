const byte ldrPin = 36; // Pin VP pada ESP32 adalah GPIO36

void setup() {
  Serial.begin(115200);
}

void loop() {
  int ldrValue = analogRead(ldrPin);

  Serial.print("Intensitas Cahaya (ADC): ");
  Serial.println(ldrValue);

  delay(1000);
}
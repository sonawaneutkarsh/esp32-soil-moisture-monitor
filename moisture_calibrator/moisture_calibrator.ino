#include <Arduino.h>

const int MOISTURE_PIN = 1;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  Serial.println("Calibration mode ready!");
  Serial.println("Stick sensor in DRY soil, wet soil, air and in a glass of water to calibrate moisture level");
}

void loop() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(MOISTURE_PIN);
    delay(10);
  }
  int raw = sum / 10;
  Serial.printf("Raw value: %d\n", raw);
  delay(1000);
}
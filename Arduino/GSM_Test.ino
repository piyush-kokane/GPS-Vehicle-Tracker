#include <SoftwareSerial.h>

SoftwareSerial sim800(A1, A0); // RX=A1, TX=A0

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  Serial.println("=== Network Test ===");
  delay(3000);

  sendAT("AT");
  sendAT("AT+CSQ");
  sendAT("AT+CCID");
  sendAT("AT+CREG?");
  sendAT("AT+COPS?");
}

void loop() {
  // check every 5 seconds
  static unsigned long last = 0;
  if (millis() - last >= 5000) {
    last = millis();
    Serial.println("--- Checking ---");
    sendAT("AT+CSQ");
    sendAT("AT+CREG?");
  }
}

void sendAT(String cmd) {
  Serial.print(">> "); Serial.println(cmd);
  sim800.println(cmd);
  delay(1000);
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
  Serial.println("---");
}
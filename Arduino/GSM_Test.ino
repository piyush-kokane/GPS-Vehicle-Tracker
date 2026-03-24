#include <SoftwareSerial.h>

SoftwareSerial sim800(A1, A0); // RX=A1, TX=A0

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  Serial.println("=== SIM800L Test ===");
  delay(3000); // wait for module to boot

  sendAT("AT");
  delay(1000);
  sendAT("AT+CSQ");
  delay(1000);
  sendAT("AT+CCID");
  delay(1000);
  sendAT("AT+CREG?");
  delay(1000);
}

void loop() {
  if (sim800.available()) Serial.write(sim800.read());
  if (Serial.available()) sim800.write(Serial.read());
}

void sendAT(String cmd) {
  Serial.print("Sending: ");
  Serial.println(cmd);
  sim800.println(cmd);
  delay(500);
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
  Serial.println("---");
}
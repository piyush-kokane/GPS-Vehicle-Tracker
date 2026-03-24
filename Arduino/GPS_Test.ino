#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial gpsSerial(A4, A3); // RX=A4, TX=A3
TinyGPSPlus gps;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Serial.println("=== GPS Test ===");
}

void loop() {
  // feed GPS data
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    Serial.write(c);
    gps.encode(c);
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();

    Serial.println(F("──────────────────────────────"));

    // Fix status
    Serial.print(F(" Fix      : "));
    Serial.println(gps.location.isValid() ? "YES ✔" : "NO ✘");

    // Location
    Serial.print(F(" Lat      : "));
    Serial.println(gps.location.isValid() ? String(gps.location.lat(), 6) : "---");

    Serial.print(F(" Lng      : "));
    Serial.println(gps.location.isValid() ? String(gps.location.lng(), 6) : "---");

    // Speed
    Serial.print(F(" Speed    : "));
    Serial.println(gps.speed.isValid() ? String(gps.speed.kmph(), 1) + " km/h" : "---");

    // Altitude
    Serial.print(F(" Altitude : "));
    Serial.println(gps.altitude.isValid() ? String(gps.altitude.meters(), 1) + " m" : "---");

    // Satellites
    Serial.print(F(" Sats     : "));
    Serial.println(gps.satellites.isValid() ? String(gps.satellites.value()) : "0");

    // Time
    Serial.print(F(" Time UTC : "));
    if (gps.time.isValid()) {
      Serial.print(gps.time.hour());   Serial.print(":");
      Serial.print(gps.time.minute()); Serial.print(":");
      Serial.println(gps.time.second());
    } else {
      Serial.println("---");
    }

    Serial.println(F("──────────────────────────────"));
  }
}

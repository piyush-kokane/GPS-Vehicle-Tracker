#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial sim800(A1, A0);    // RX=A1, TX=A0
SoftwareSerial gpsSerial(A4, A3); // RX=A4, TX=A3
TinyGPSPlus gps;

int satsInView = 0;
bool gsmLog = true;
bool gprsConnected = false;
unsigned long lastGPRSTry  = 0;
const unsigned long GPRS_RETRY = 30000;

// Firebase settings
String firebaseHost = "gps-vehicle-tracker-a538c-default-rtdb.asia-southeast1.firebasedatabase.app";
String userKey      = "pjkokane21@gmail_com";
String firebasePath = "/" + userKey + "/current_loc.json";

// forward declaration
void handleCommand(String cmd);
bool initGPRS();
void readGPS();
String getAT(String cmd, int timeout = 1000);
bool pushToDB(float lat, float lng, float spd, float alt, String time, String date);



void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  gpsSerial.begin(9600);
  delay(3000);
  
  sim800.listen();
  sim800.println("ATE0"); // disable echo
  delay(1000);
  while(sim800.available()) sim800.read(); // flush

  // HEADER
  Serial.println();
  Serial.println(F("=== GPS Vehicle Tracker =================================================="));
  Serial.println();

  // COMMANDS
  Serial.println(F("--- COMMANDS ---"));
  Serial.print(F(" ● Toggle GSM Logs : ")); Serial.println(F("gsmlog"));
  Serial.print(F(" ● Reconnect GPRS  : ")); Serial.println(F("reconnect"));
  Serial.print(F(" ● Reset SIM800L   : ")); Serial.println(F("reset"));
  Serial.println();

  // NETWORK STATUS
  Serial.println(F("--- NETWORK STATUS ---"));
  if (gsmLog) {
    String cops  = getAT("AT+COPS?");
    String _cops = getAT("AT+COPS=?", 45000);

    Serial.print(F(" ● Current   : ")); Serial.println(cops);
    Serial.print(F(" ● Available : ")); Serial.println(_cops);
  }
  else {
    Serial.println(F(" ⚠︎ GSM Logs Disabled"));
  }

  // INIT GPRS
  Serial.print(gsmLog ? " ● GPRS      : " : " ● GPRS : ");
  gprsConnected = initGPRS();
  lastGPRSTry   = millis();
  Serial.println(gprsConnected ? "Connected ✔" : "Failed ✘");
}



void loop() {
  // check for serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    handleCommand(cmd);
  }

  // Read GPS
  readGPS();
  
  // GSM STATUS - Switch to GSM
  Serial.println();
  Serial.println(F("──────────────────────────────"));
  Serial.println();
  Serial.println(F("--- SIM800L STATUS  ---"));
  if (gsmLog) {
    String csq  = getAT("AT+CSQ");
    String ccid = getAT("AT+CCID");
    String creg = getAT("AT+CREG?");

    Serial.print(F(" ● Signal  (+CSQ) : ")); Serial.println(csq);
    Serial.print(F(" ● SIM     (CCID) : ")); Serial.println(ccid);
    Serial.print(F(" ● Network (CREG) : ")); Serial.println(creg);
  }
  else {
    Serial.println(F(" ⚠︎ GSM Logs Disabled "));
  }

  // GPRS STATUS
  Serial.print(gsmLog ? " ● GPRS           : " : " ● GPRS : ");
  if (gprsConnected) {
    Serial.println(F("Connected ✔"));
  } else if (millis() - lastGPRSTry >= GPRS_RETRY) {
    lastGPRSTry   = millis();
    gprsConnected = initGPRS();
    Serial.println(gprsConnected ? "Connected ✔" : "Failed ✘");
  } else {
    Serial.print(F("Retry in "));
    Serial.print((GPRS_RETRY - (millis() - lastGPRSTry)) / 1000);
    Serial.println(F("s"));
  }

  // GPS STATUS
  Serial.println();
  Serial.println(F("--- GPS STATUS  ---"));

  if (gps.location.isValid()) {
    float  lat  = gps.location.lat();
    float  lng  = gps.location.lng();
    float  spd  = gps.speed.isValid()       ? gps.speed.kmph()       : 0.0;
    float  alt  = gps.altitude.isValid()    ? gps.altitude.meters()  : 0.0;
    int    sats = gps.satellites.isValid()  ? gps.satellites.value() : 0;

    String time = gps.time.isValid()        ? String(gps.time.hour())   + ":" +
                                              String(gps.time.minute()) + ":" +
                                              String(gps.time.second())
                                            : "---";

    String date = gps.date.isValid()        ? String(gps.date.day())   + "/" +
                                              String(gps.date.month()) + "/" +
                                              String(gps.date.year())
                                            : "---";

    Serial.println(F(" ✔ Fix: YES"));
    Serial.print(F(" ● Latitude   : ")); Serial.println(lat, 6);
    Serial.print(F(" ● Longitude  : ")); Serial.println(lng, 6);
    Serial.print(F(" ● Speed km/h : ")); Serial.println(spd, 1);
    Serial.print(F(" ● Altitude m : ")); Serial.println(alt, 1);
    Serial.print(F(" ● Satellites : ")); Serial.println(sats);
    Serial.print(F(" ● Time UTC   : ")); Serial.println(time);
    Serial.print(F(" ● Firebase   : "));

    if (gprsConnected) {
      bool pushed = pushToDB(lat, lng, spd, alt, time, date);
      if (!pushed) {
        gprsConnected = false;
        lastGPRSTry   = 0;
      }
      Serial.println(pushed ? "Pushed ✔" : "Failed ✘");
    } else {
      Serial.println(F("Waiting for GPRS..."));
    }
  }
  else {
    Serial.println(F(" ✘ Fix: NO"));
    Serial.print(F(" ● Satellites in view : ")); Serial.println(satsInView);
  }

  Serial.println();
  Serial.println(F("──────────────────────────────"));
  
  delay(2000);
}



// Handle Serial Commands
void handleCommand(String cmd) {
  if (cmd == "gsmlog") {
    gsmLog = !gsmLog;
    Serial.print(F("GSM Logs : "));
    Serial.println(gsmLog ? "Enabled ✔" : "Disabled ✘");
  }
  else if (cmd == "reconnect") {
    Serial.print(F("Reconnecting GPRS: "));
    gprsConnected = initGPRS();
    lastGPRSTry   = millis();
    Serial.println(gprsConnected ? "Connected ✔" : "Failed ✘");
  }
  else if (cmd == "reset") {
    Serial.println(F("Resetting SIM800L..."));
    getAT("AT+CFUN=1,1", 5000);
    gprsConnected = false;
    lastGPRSTry   = 0;
    Serial.println(F("Reset sent — wait 5s then reconnect"));
  }
  else {
    Serial.println(F(" ⚠︎ Unrecognized Command"));
  }
}



// Init GPRS
bool initGPRS() {
  getAT("AT+CGATT=1");
  getAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  getAT("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"");
  getAT("AT+SAPBR=1,1", 5000);
  String bearer = getAT("AT+SAPBR=2,1");
  if (bearer.indexOf("1,1") != -1) return true;
  return false;
}



// Read GPS
void readGPS() {
  gpsSerial.listen();
  static String gpsLine = "";
  unsigned long start = millis();
  while (millis() - start < 500) {
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      gps.encode(c);
      gpsLine += c;
      if (c == '\n') {
        if (gpsLine.startsWith("$GPGSV")) {
          int a  = gpsLine.indexOf(',');
          int b  = gpsLine.indexOf(',', a + 1);
          int c2 = gpsLine.indexOf(',', b + 1);
          int d  = gpsLine.indexOf(',', c2 + 1);
          int val = gpsLine.substring(c2 + 1, d).toInt();
          if (val > 0) satsInView = val;
        }
        gpsLine = "";
      }
    }
  }
}



// AT commands
String getAT(String cmd, int timeout = 1000) {
  sim800.listen();
  sim800.println(cmd);
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (sim800.available()) {
      char c = sim800.read();
      response += c;
    }
    if (response.indexOf("OK") != -1) break;
    if (response.indexOf("ERROR") != -1) return "ERROR";
  }

  // for COPS? extract name between quotes
  if (cmd == "AT+COPS?") {
    int first = response.indexOf('"');
    int last  = response.lastIndexOf('"');
    if (first != -1 && last != -1 && first != last) {
      return response.substring(first + 1, last);
    } else {
      return "Not registered";
    }
  }

  // clean response
  response.replace("OK", "");
  response.replace("ERROR", "");
  response.replace("+CSQ: ", "");
  response.replace("+CREG: ", "");
  response.replace("+CCID: ", "");
  response.replace("+COPS: ", "");
  response.replace("\r", "");
  response.replace("\n", "");
  response.trim();
  return response;
}



// Push to Firebase
bool pushToDB(float lat, float lng, float spd, float alt, String time, String date) {
  String timeStr = date + " " + time;

  String data = "{\"lat\":"    + String(lat, 6) +
                ",\"lng\":"    + String(lng, 6) +
                ",\"spd\":"    + String(spd, 1) +
                ",\"alt\":"    + String(alt, 1) +
                ",\"time\":\"" + timeStr + "\"}";

  // init HTTP
  if (getAT("AT+HTTPINIT") == "ERROR") return false;
  getAT("AT+HTTPPARA=\"CID\",1");
  getAT("AT+HTTPPARA=\"URL\",\"https://" + firebaseHost + firebasePath + "\"");
  getAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"");

  // send data
  sim800.listen();
  sim800.println("AT+HTTPDATA=" + String(data.length()) + ",5000");
  delay(1000);
  sim800.println(data);
  delay(1000);

  // POST
  sim800.println("AT+HTTPACTION=1");
  unsigned long start = millis();
  String action = "";
  while (millis() - start < 10000) {
    while (sim800.available()) {
      action += (char)sim800.read();
    }
    if (action.indexOf("+HTTPACTION") != -1) break;
  }

  getAT("AT+HTTPTERM");

  if (action.indexOf(",200,") != -1) return true;
  return false;
}

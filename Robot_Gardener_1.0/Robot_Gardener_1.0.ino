#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <WiFiManager.h>
#include "time.h"
#include <FirebaseESP32.h>

#include <Addons/TokenHelper.h>
#include <Addons/RTDBHelper.h>

#define ESP_AP_NAME "boken 8266"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino
#define TOKEN "P1lqN7qC8xCIzSPcPxkaQHpbghBsaQe0JP3V60UQ"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://robotgardener-qfuu-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

int pump = 1;
int led = 3;

bool pumpOn = true;
bool pumpOff = false;

bool ledOn = true;
bool ledOff = false;

const char* ntpServer = "th.pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 25200;

void setup() {
  Serial.begin(9600);

  WiFiManager wifiManager;
  wifiManager.autoConnect(ESP_AP_NAME);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  config.api_key = TOKEN;

  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third-party library
  Firebase.reconnectWiFi(true);

  //time for start/stop Pump
  Firebase.RTDB.setInt(&fbdo, "/value/time/moisture/start/hour", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/time/moisture/start/minute", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/time/moisture/stop/hour", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/time/moisture/stop/minute", 0);
  //time for start/stop Led
  Firebase.RTDB.setInt(&fbdo, "/value/time/led/start/hour", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/time/led/start/minute", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/time/led/stop/hour", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/time/led/stop/minute", 0);

  Firebase.RTDB.setInt(&fbdo, "/value/data/moisture/setmoisture", 0);
  Firebase.RTDB.setInt(&fbdo, "/value/data/light/setlight", 0);

  pinMode(pump, OUTPUT);
  pinMode(led, OUTPUT);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  int currentmoisture = map(analogRead(14), 4095, 400, 0, 100);
  Serial.println(currentmoisture);
  int currentlight = map(analogRead(12), 250, 1023, 0, 100);
  Serial.println(currentlight);
  delay(500);
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {

    sendDataPrevMillis = millis();
    
    int startmh = (Firebase.RTDB.getInt(&fbdo, "/value/time/moisture/start/hour"),int(fbdo.intData()));
    int startmm = (Firebase.RTDB.getInt(&fbdo, "/value/time/moisture/start/minute"),int(fbdo.intData()));
    int stopmh = (Firebase.RTDB.getInt(&fbdo, "/value/time/moisture/stop/hour"),int(fbdo.intData()));
    int stopmm = (Firebase.RTDB.getInt(&fbdo, "/value/time/moisture/stop/minute"),int(fbdo.intData()));

    int startlh = (Firebase.RTDB.getInt(&fbdo, "/value/time/led/start/hour"),int(fbdo.intData()));
    int startlm = (Firebase.RTDB.getInt(&fbdo, "/value/time/led/start/minute"),int(fbdo.intData()));
    int stoplh = (Firebase.RTDB.getInt(&fbdo, "/value/time/led/stop/hour"),int(fbdo.intData()));
    int stoplm = (Firebase.RTDB.getInt(&fbdo, "/value/time/led/stop/minute"), int(fbdo.intData()));

    Serial.println(startmh);
    Serial.println(startmm);

    int pumpStartTime = startmh * 60 + startmm;
    int pumpStopTime = stopmh * 60 + stopmm;

    int ledStartTime = startlh * 60 + startlm;
    int ledStopTime = stoplh * 60 + stoplm;

    int setmoisture = (Firebase.RTDB.getInt(&fbdo, "/value/data/moisture/setmoisture"),int(fbdo.intData()));
    int setlight = (Firebase.RTDB.getInt(&fbdo, "/value/data/light/setlight"),int(fbdo.intData()));

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }

    int currentMinutes = (timeinfo.tm_hour * 60) + timeinfo.tm_min;

    Firebase.RTDB.setInt(&fbdo, "/value/data/moisture/currentmoisture", currentmoisture);
    Firebase.RTDB.setInt(&fbdo, "/value/data/light/currentlight", currentlight);

    //Pump On/Off when currentmoisture > setmoisture
    if (currentmoisture < setmoisture && currentMinutes >= pumpStartTime && currentMinutes < pumpStopTime) {
      Firebase.RTDB.setBool(&fbdo, "/value/Pump/PumpOn", true);
      pumpOn = true;
      delay(200);
    } else {
      Firebase.RTDB.setBool(&fbdo, "/value/Pump/PumpOn", false);
      pumpOn = false;
      delay(200);
    }

    //led On/Off when currentlight > setlight
    if (currentlight > setlight && currentMinutes >= ledStartTime && currentMinutes < ledStopTime) {
      Firebase.RTDB.setBool(&fbdo, "/value/Led/LedOn", true);
      ledOn = true;
      delay(200);
    } else {
      Firebase.RTDB.setBool(&fbdo, "/value/Led/LedOn", false);
      ledOn = false;
      delay(200);
    }

    //Pump On when Pump/PumpOn is True
    if (pumpOn == true) {
      digitalWrite(pump, HIGH);
      delay(100);
    } else {
      digitalWrite(pump, LOW);
      delay(100);
    }

    //led On when Led/LedOn is True
    if (ledOn == true) {
      digitalWrite(led, HIGH);
      delay(100);
    } else {
      digitalWrite(led, LOW);
      delay(100);
    }
  }
}

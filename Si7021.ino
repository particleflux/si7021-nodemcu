#include "config.h"
#include "Adafruit_Si7021.h"

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Ticker.h>


ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
Adafruit_Si7021 sensor = Adafruit_Si7021();
bool isSensorAvailable = true;
unsigned long lastPublish = 0;
float humidity, temperature;


void setup() {
  Ticker blink;
  
  // setup builtin LED GPIO
  pinMode(D4, OUTPUT);
  // turn LED ON
  digitalWrite(D4, LOW);
  
  Serial.begin(115200);
  Serial.println("\n\nSi7021 test!");
  
  // rapid blinking while connecting and getting ready for OTA
  blink.attach(0.1, ledBlink);
  setupOTA();
  blink.detach();
  // turn LED OFF
  digitalWrite(D4, HIGH);
  
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    isSensorAvailable = false;
    return;
  }
}

void loop() {
  unsigned long ms = millis();

  httpServer.handleClient();
  MDNS.update();
  
  if (!isSensorAvailable) {
    delay(1000);
    return;
  }

  if (ms - lastPublish < PUBLISH_INTERVAL) {
    return;
  }

  update();
  
  Serial.print("Humidity:    ");
  Serial.print(humidity, 2);
  Serial.print("\tTemperature: ");
  Serial.println(temperature, 2);
  delay(1000);
}


void setupOTA() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(MY_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  MDNS.begin(MY_HOSTNAME);

  httpUpdater.setup(&httpServer, OTA_USERNAME, OTA_PASSWORD);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

void update() {
  humidity = sensor.readHumidity();
  temperature = sensor.readTemperature();

  lastPublish = millis();
}

void ledBlink() {
  digitalWrite(D4, !digitalRead(D4));
}


#include "config.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Ticker.h>

#include "Adafruit_Si7021.h"


/******
 * Need to change PubSubClient.h version!
 * ~/Arduino/libraries/PubSubClient/src/PubSubClient.h
 * 
// MQTT_VERSION : Pick the version
#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION MQTT_VERSION_3_1_1
#endif




SCL -> D1
SDA -> D2


 */

Ticker blink;
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_Si7021 sensor = Adafruit_Si7021();
IPAddress statsdIP(STATSD_IP);
WiFiUDP udp;
bool isSensorAvailable = true;
unsigned long lastPublish = 0;
char buffer[128];
float humidity, temperature;


void ledBlink() {
  int state = digitalRead(D4);
  digitalWrite(D4, !state);
}

void setup() {
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  
  Serial.begin(115200);
  Serial.println("Si7021 test!");
  
  if (!sensor.begin()) {
    // we have a solid blue LED when sensor not found
    Serial.println("Did not find Si7021 sensor!");
    isSensorAvailable = false;
    return;
  }

  // rapid blinking while connecting
  blink.attach(0.2, ledBlink);
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());


  client.setServer(MQTT_HOST, 1883);

  blink.detach();
//  blink.attach(1.0, ledBlink);
}

void loop() {
  if (!isSensorAvailable) {
    delay(1000);
    return;
  }

  unsigned long ms = millis();

  if (client.connected()) {
    client.loop();
  } else {
    client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
  }

  if (ms - lastPublish < PUBLISH_INTERVAL) {
    return;
  }

  update();
  
  Serial.print("Humidity:    "); Serial.print(humidity, 2);
  Serial.print("\tTemperature: "); Serial.println(temperature, 2);
  delay(1000);
}


void update() {
  humidity = sensor.readHumidity();
  temperature = sensor.readTemperature();

  lastPublish = millis();

  snprintf(buffer, sizeof(buffer), "{\"temperature\": %f, \"humidity\": %f, \"time\": 0}", temperature, humidity);
  client.publish(MQTT_PUBLISH_TOPIC, buffer);

  snprintf(buffer, sizeof(buffer), "home.temperature,room=livingroom:%.2f|g\nhome.humidity,room=livingroom:%.2f|g\n", temperature, humidity);
  udp.beginPacket(statsdIP, STATSD_PORT);
  udp.write(buffer);
  udp.endPacket();
}


#include "config.h"
#include "Adafruit_Si7021.h"

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <NTPClient.h>

/******
 * Need to change PubSubClient.h version!
 * ~/Arduino/libraries/PubSubClient/src/PubSubClient.h
 * Remove the comment on the 3_1 definition
 *
 * // MQTT_VERSION : Pick the version
 * #define MQTT_VERSION MQTT_VERSION_3_1
 * #ifndef MQTT_VERSION
 * #define MQTT_VERSION MQTT_VERSION_3_1_1
 * #endif
 */


ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
Adafruit_Si7021 sensor = Adafruit_Si7021();
bool isSensorAvailable = true;
unsigned long lastPublish = 0;
float humidity, temperature;
IPAddress statsdIP(STATSD_IP);
WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, NTP_INTERVAL);

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

  // set MQTT server
  client.setServer(MQTT_HOST, 1883);

  // setup NTP client
  timeClient.begin();
}

void loop() {
  unsigned long ms = millis();

  httpServer.handleClient();
  MDNS.update();
  
  if (!isSensorAvailable) {
    delay(1000);
    return;
  }

  // keep NTP time up-to-date - this may block for some seconds in case it does an update
  timeClient.update();

  if (client.connected()) {
     client.loop();
  } else {
    Serial.print("MQTT disconnected, re-connecting...");
    client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
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
  WiFiUDP udp;
  char buffer[128];

  humidity = sensor.readHumidity();
  temperature = sensor.readTemperature();

  lastPublish = millis();

  // publish StatsD metrics
  snprintf(
    buffer,
    sizeof(buffer),
    "home.temperature,room=livingroom:%.2f|g\nhome.humidity,room=livingroom:%.2f|g\n",
    temperature,
    humidity
   );

  udp.beginPacket(statsdIP, STATSD_PORT);
  udp.write(buffer);
  udp.endPacket();


  // publish MQTT message
  snprintf(
    buffer,
    sizeof(buffer),
    "{\"temperature\": %f, \"humidity\": %f, \"time\": %u}",
    temperature,
    humidity,
    timeClient.getEpochTime()
  );
  client.publish(MQTT_PUBLISH_TOPIC, buffer);
}

void ledBlink() {
  digitalWrite(D4, !digitalRead(D4));
}


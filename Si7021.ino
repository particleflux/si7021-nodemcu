#include "config.h"
#include "Adafruit_Si7021.h"


Adafruit_Si7021 sensor = Adafruit_Si7021();
bool isSensorAvailable = true;
unsigned long lastPublish = 0;
float humidity, temperature;


void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSi7021 test!");
  
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    isSensorAvailable = false;
    return;
  }
}

void loop() {
  unsigned long ms = millis();
  
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


void update() {
  humidity = sensor.readHumidity();
  temperature = sensor.readTemperature();

  lastPublish = millis();
}


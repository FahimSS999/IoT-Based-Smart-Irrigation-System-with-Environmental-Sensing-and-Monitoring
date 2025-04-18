#define BLYNK_TEMPLATE_ID "TMPL619UERD7z"
#define BLYNK_TEMPLATE_NAME "Smart Agriculture"
#define BLYNK_AUTH_TOKEN "4-Bo8_PJEARC7CNHwF8dWkv_WGo5wtrO"

#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>

// === User Configurable Settings ===
char ssid[] = "your_wifi_name";
char pass[] = "your_wifi_password";

// === Easily Configurable Pins ===
const int SOIL_MOISTURE_PIN = 32;
const int DHT_PIN = 4;
const int DHT_TYPE = DHT11;
const int WATER_SIGNAL_PIN = 36;
const int WATER_POWER_PIN = 17;
const int RELAY_PIN = 26;

// === Objects & Variables ===
DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;

int counter = 0;
bool relayManualState = false;

void sendSensorData() {
  Serial.println("Reading #" + String(counter++));

  // Soil Moisture
  int soilAnalog = analogRead(SOIL_MOISTURE_PIN);
  int moisture = constrain(100 - ((soilAnalog / 4095.0) * 100), 0, 100);

  // Temp & Humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Water Level
  digitalWrite(WATER_POWER_PIN, HIGH);
  delay(10);
  int waterRaw = analogRead(WATER_SIGNAL_PIN);
  digitalWrite(WATER_POWER_PIN, LOW);
  float waterPercent = min((waterRaw / 2000.0) * 100.0, 100.0f);

  // Debug
  Serial.printf("Soil Moisture: %d%%\n", moisture);
  Serial.printf("Temp: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
  Serial.printf("Water Level: %.1f%%\n", waterPercent);

  // Send to Blynk
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, moisture);
  Blynk.virtualWrite(V3, waterPercent);

  // Events
  if (temperature > 30.0)
    Blynk.logEvent("high_temperature", "Temp > 30°C");
  if (humidity > 60.0)
    Blynk.logEvent("high_humidity", "Humidity > 60%");
  if (moisture < 30)
    Blynk.logEvent("low_moisture", "Soil moisture < 30%");
  if (waterPercent < 30.0)
    Blynk.logEvent("water_ending", "Water < 30%");

  // Auto Pump OFF
  if (moisture >= 75) {
    digitalWrite(RELAY_PIN, HIGH); // OFF
    Blynk.virtualWrite(V4, 0);     // Update App
    relayManualState = false;
    Serial.println("Soil Moisture > 75% → Auto Pump OFF");
  }

  Serial.println("------------------------------------------");
}

// Relay manual control from app
BLYNK_WRITE(V4) {
  int relayState = param.asInt();
  relayManualState = relayState;
  digitalWrite(RELAY_PIN, relayState == 1 ? LOW : HIGH); // LOW=ON, HIGH=OFF
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogSetAttenuation(ADC_11db);

  pinMode(WATER_POWER_PIN, OUTPUT);
  digitalWrite(WATER_POWER_PIN, LOW);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // OFF initially

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}

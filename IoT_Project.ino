#define BLYNK_TEMPLATE_ID "TMPL619UERD7z"
#define BLYNK_TEMPLATE_NAME "Smart Agriculture"
#define BLYNK_AUTH_TOKEN "4-Bo8_PJEARC7CNHwF8dWkv_WGo5wtrO"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// --- WiFi credentials ---
char ssid[] = "sakib";
char pass[] = "sakib123456";

// --- Pin Definitions ---
const int soilMoisturePin = 32;
#define DHTPIN 4
#define DHTTYPE DHT11
#define POWER_PIN 17
#define SIGNAL_PIN 36
const int relay = 26;

// --- Sensor Variables ---
int soilAnalog = 0;
int moisture = 0;
float temperature = 0.0;
float humidity = 0.0;
int waterRaw = 0;
float waterPercent = 0.0;
int counter = 0;
bool relayManualState = false;

// --- Objects ---
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// --- Relay Control via Virtual Pin V4 ---
BLYNK_WRITE(V4) {
  int relayState = param.asInt();
  relayManualState = relayState;
  digitalWrite(relay, relayState == 1 ? LOW : HIGH); // LOW = ON, HIGH = OFF (NO Relay)
}

void sendSensorData() {
  Serial.println("Reading #" + String(counter++));

  // --- Soil Moisture ---
  soilAnalog = analogRead(soilMoisturePin);
  moisture = 100 - ((soilAnalog / 4095.0) * 100);
  moisture = constrain(moisture, 0, 100);

  // --- Temperature & Humidity ---
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // --- Water Level ---
  digitalWrite(POWER_PIN, HIGH);
  delay(10);
  waterRaw = analogRead(SIGNAL_PIN);
  digitalWrite(POWER_PIN, LOW);
  waterPercent = (waterRaw / 2000.0) * 100.0;
  waterPercent = min(waterPercent, 100.0f);

  // --- Serial Debug Output ---
  Serial.printf("Soil Moisture: %d%%\n", moisture);
  Serial.printf("Temp: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
  Serial.printf("Water Level: %.1f%%\n", waterPercent);

  // --- Send to Blynk ---
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, moisture);
  Blynk.virtualWrite(V3, waterPercent);

  // --- Event Notifications ---
  if (temperature > 30.0) {
    Blynk.logEvent("high_temperature", "Temperature is above 30°C");
  }
  if (humidity > 60.0) {
    Blynk.logEvent("high_humidity", "Humidity is above 60%");
  }
  if (moisture < 30) {
    Blynk.logEvent("low_moisture", "Soil moisture is below 30%");
  }
  if (waterPercent < 30.0) {
    Blynk.logEvent("water_ending", "Water level is below 30%");
  }

  // --- Auto-Pump OFF ---
  if (moisture >= 75) {
    digitalWrite(relay, HIGH);  // Turn OFF Pump
    Blynk.virtualWrite(V4, 0);  // Update Virtual Button to OFF
    relayManualState = false;
    Serial.println("Soil Moisture > 75% → Auto Pump OFF");
  }

  Serial.println("------------------------------------------------");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogSetAttenuation(ADC_11db);

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);

  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH); // Relay OFF initially

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, sendSensorData); // Read every 2 sec
}

void loop() {
  Blynk.run();
  timer.run();
}
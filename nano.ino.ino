#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFiNINA.h>
#include <PubSubClient.h>

// WiFi and MQTT details
const char* ssid = "Redmi Note 10 Pro";       // Replace with your WiFi network name
const char* password = "ryan@123"; // Replace with your WiFi password
const char* mqtt_server = "broker.emqx.io"; // Free MQTT broker or your local broker

WiFiClient wifiClient;
PubSubClient client(wifiClient);

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
long lastSent;

float beatsPerMinute;
int beatAvg;
const int thermistorPin = A7; // Thermistor connected to A7
float temperatureF; // Temperature in Fahrenheit

// Thermistor constants
const float BETA = 3950; // Beta value of thermistor
const float ROOM_TEMP_RESISTANCE = 10000; // Resistance at 25°C

// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Start connection to WiFi
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to the MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ArduinoNano33IOT")) { // Client ID
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Function to calculate temperature in Fahrenheit from thermistor readings
float getTemperatureF() {
  int analogValue = analogRead(thermistorPin);
  float resistance = (1023.0 / analogValue - 1) * ROOM_TEMP_RESISTANCE;
  float temperatureC = 1 / (log(resistance / ROOM_TEMP_RESISTANCE) / BETA + 1 / 298.15) - 273.15;
  return (temperatureC * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize Wi-Fi and MQTT connection
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // Use default I2C port, 400kHz speed
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }

  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    // We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }
  

  // Calculate temperature
  temperatureF = getTemperatureF();

  if (millis() - lastSent > 1000) {
    if ((beatsPerMinute >= 40 && beatsPerMinute <= 180) && 
        (temperatureF >= 70.0 && temperatureF <= 104.0)) {
      
      String message = String(temperatureF) + "," + String(beatsPerMinute);
      client.publish("SB_PI_DATA", message.c_str());
      lastSent = millis();
      
         Serial.println("Published valid reading to Raspberry Pi");
    } else {
      Serial.println("Skipped due to out-of-range reading");
    }
  }
// Print the sensor readings and temperature
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.print(", TempF=");
  Serial.print(temperatureF);

  if (irValue < 50000) {
    Serial.print(" No finger?");
    beatsPerMinute = 0;
  }

  Serial.println();
}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

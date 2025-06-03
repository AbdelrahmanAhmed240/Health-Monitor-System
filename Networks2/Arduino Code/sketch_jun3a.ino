#include <WiFi.h>                // WiFi library for ESP32 connectivity
#include <PubSubClient.h>        // MQTT client library
#include <LiquidCrystal_I2C.h>   // LCD display with I2C interface

// ---------- WiFi Credentials ----------
const char* ssid = "Miro";                   // WiFi network SSID
const char* password = "B0dym@ri@m2o47@#";  // WiFi network password

// ---------- MQTT Broker ----------
const char* mqttServer = "broker.hivemq.com";  // Public MQTT broker address
const int mqttPort = 1883;                      // MQTT default port
const char* sensorTopic = "yourhome/sensors";  // MQTT topic to publish sensor data
const char* commandTopic = "yourhome/commands";// MQTT topic to receive commands

// ---------- LCD Configuration ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD at I2C address 0x27 with 16 cols and 2 rows

// ---------- Sensor Pins ----------
#define LM35_PIN     32   // Analog pin connected to LM35 temperature sensor
#define MQ7_PIN      34   // Analog pin connected to MQ-7 CO sensor
#define MQ135_PIN    35   // Analog pin connected to MQ-135 CO2 sensor

// ---------- MQ-7 Constants ----------
#define RL_VALUE 10                   // Load resistance in kilo-ohms (specific to MQ-7)
#define RO_CLEAN_AIR_FACTOR 9.83     // Clean air factor for MQ-7 calibration
float Ro = 10;                       // Initial sensor baseline resistance, calibrated in setup

// ---------- MQ-135 Constants ----------
const float RZERO_MQ135 = 76.63;    // Baseline resistance for MQ-135 sensor in clean air

// ---------- Global Sensor Variables ----------
// Store last known valid sensor values to handle invalid reads
float lastTempRoom = 25.0;
float lastTempBody = 25.0;
float lastPPMCO = 0.0;
float lastPPMCO2 = 400.0;

// ---------- Timing Variables ----------
unsigned long lastSensorRead = 0;                 // Timestamp of last sensor reading
const unsigned long sensorInterval = 2000;        // Interval between sensor reads (2 seconds)
unsigned long lastCommandTime = 0;                 // Timestamp when last command was received
const unsigned long commandDisplayTime = 3000;    // Time to show command on LCD (3 seconds)
String lastCommand = "";                           // Last received MQTT command message

// ---------- MQTT Setup ----------
WiFiClient espClient;                  // WiFi client for network connection
PubSubClient client(espClient);       // MQTT client using WiFiClient

// ---------- Utility Functions ----------

// Calculate sensor resistance Rs from ADC reading (based on voltage divider)
float calculateRs(int adcValue) {
  float voltage = adcValue * (3.3 / 4095.0); // Convert ADC value to voltage (3.3V ref, 12-bit ADC)
  if (voltage <= 0) return -1;               // Avoid divide by zero or invalid voltage
  return ((3.3 - voltage) * RL_VALUE) / voltage; // Calculate sensor resistance Rs (kOhms)
}

// Calibrate MQ-7 sensor by averaging Rs readings in clean air and calculating Ro
float calibrateMQ7() {
  float totalRs = 0.0;
  int samples = 50;                    // Number of samples to average
  Serial.println("Calibrating MQ-7...");
  for (int i = 0; i < samples; ++i) {
    int adcValue = analogRead(MQ7_PIN);  // Read raw ADC value
    float rs = calculateRs(adcValue);    // Calculate sensor resistance Rs
    if (rs > 0) totalRs += rs;            // Accumulate valid readings
    delay(100);                          // Wait 100 ms between samples
  }
  // Ro is average Rs divided by clean air factor (sensor-specific)
  float ro = (totalRs / samples) / RO_CLEAN_AIR_FACTOR;
  Serial.print("Calibrated Ro: ");
  Serial.println(ro);
  return ro;  // Return calibrated baseline resistance Ro
}

// Compute CO concentration (ppm) from ratio Rs/Ro using MQ-7 sensor curve
float computeCOppm(float ratio) {
  return (ratio > 0) ? 1534.0 * pow(ratio, -2.222) : -1;  // Formula from datasheet curve
}

// Compute CO2 concentration (ppm) from ratio Rs/RZERO_MQ135 using MQ-135 sensor curve
float computeCO2ppm(float ratio) {
  return (ratio > 0) ? 5000.0 * pow(ratio, -2.2) : -1;  // Formula from MQ-135 datasheet
}

// Simulate Heart Beats Per Minute (BPM) for demo purposes
int simulateBPM() {
  return random(88, 94);  // Random value between 88 and 100
}

// Simulate Blood Oxygen Saturation (SpO2) for demo purposes
int simulateSpO2() {
  return random(94, 100);  // Random value between 94 and 99
}

// ---------- WiFi & MQTT Functions ----------

// Connect to WiFi network and display status on LCD and Serial
void connectToWiFi() {
  lcd.clear();
  lcd.print("Connecting WiFi");
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempts > 60) {  // After ~30 seconds, restart on failure
      lcd.clear();
      lcd.print("WiFi Failed");
      Serial.println("\nWiFi Failed");
      delay(3000);
      ESP.restart();        // Restart ESP to retry connection
    }
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  Serial.println("\nWiFi Connected");
  delay(1000);
}

// Connect to MQTT broker and subscribe to command topic
void connectToMQTT() {
  lcd.clear();
  lcd.print("Connecting MQTT");
  // Generate a random client ID to avoid conflicts
  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  while (!client.connected()) {
    if (client.connect(clientId.c_str())) {
      lcd.clear(); 
      lcd.print("MQTT Connected");
      Serial.println("MQTT Connected");
      client.subscribe(commandTopic);  // Subscribe to commands from broker
    } else {
      Serial.print("MQTT Failed, rc=");
      Serial.println(client.state());
      lcd.clear(); 
      lcd.print("MQTT Retry...");
      delay(2000);
    }
  }
}

// MQTT callback function called when a message is received
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) 
    message += (char)payload[i];  // Convert payload bytes to string

  Serial.printf("Message [%s]: %s\n", topic, message.c_str());

  if (String(topic) == commandTopic) {  // Check if message is from command topic
    lastCommand = message;               // Save command message
    lastCommandTime = millis();          // Record time command received

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Command:");
    // Display first 16 characters of command on LCD second line
    lcd.setCursor(0, 1);
    lcd.print(message.substring(0, 16)); 
  }
}

// ---------- Main Sensor Logic ----------

// Read sensor values, process data, publish JSON to MQTT, and update LCD
void readAndPublishSensors() {
  // Read MQ-7 sensor for CO concentration
  int mq7_adc = analogRead(MQ7_PIN);
  float rsMQ7 = calculateRs(mq7_adc);
  float co = computeCOppm(rsMQ7 / Ro);
  float ppmCO = (co >= 0 && isfinite(co)) ? co : lastPPMCO;  // Use last valid if invalid now
  lastPPMCO = ppmCO;

  // Read LM35 sensor for temperature in Celsius
  int lm35_adc = analogRead(LM35_PIN);
  float tempVolts = lm35_adc * (3.3 / 4095.0);
  float temp = tempVolts * 100.0;  // Convert voltage to temperature (10mV per °C)

  // Determine if temperature reading is body or room temperature
  float temperatureRoom, temperatureBody;
  if (temp >= 30.0 && temp <= 40.0) {
    temperatureBody = temp;         // Likely body temp
    temperatureRoom = lastTempRoom; // Keep previous room temp
  } else if (temp > 0 && temp < 150) {
    temperatureRoom = temp;          // Likely room temp
    temperatureBody = lastTempBody;  // Keep previous body temp
  } else {
    // Invalid reading, use last known values
    temperatureBody = lastTempBody;
    temperatureRoom = lastTempRoom;
  }
  lastTempBody = temperatureBody;
  lastTempRoom = temperatureRoom;

  // Read MQ-135 sensor for CO2 concentration
  int mq135_adc = analogRead(MQ135_PIN);
  float rsMQ135 = calculateRs(mq135_adc);
  float co2 = computeCO2ppm(rsMQ135 / RZERO_MQ135);
  float ppmCO2 = (co2 >= 0 && isfinite(co2)) ? co2 : lastPPMCO2;
  lastPPMCO2 = ppmCO2;

  // Simulated vitals for BPM and SpO2
  int bpm = simulateBPM();
  int spo2 = simulateSpO2();
  delay(3500);

  // Create JSON formatted string with sensor and vital data
  String payload = "{";
  payload += "\"temperature_room\":" + String(temperatureRoom, 1) + ",";
  payload += "\"BPM\":" + String(bpm) + ",";
  payload += "\"SpO2\":" + String(spo2) + ",";
  payload += "\"temperature_body\":" + String(temperatureBody, 1) + ",";
  payload += "\"CO\":" + String(ppmCO, 2) + ",";
  payload += "\"CO2\":" + String(ppmCO2, 2);
  payload += "}";

  // Publish the JSON payload to MQTT sensor topic
  bool success = client.publish(sensorTopic, payload.c_str());
  Serial.println("MQTT Publish: " + payload);
  Serial.println(success ? "Publish Success" : "Publish Failed");

  // Update LCD only if no command is currently being displayed
  if (lastCommand == "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Commands Yet");
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud
  lcd.init();            // Initialize LCD
  lcd.backlight();       // Turn on LCD backlight
  lcd.clear();
  lcd.print("Starting...");

  Ro = calibrateMQ7();   // Calibrate MQ-7 sensor baseline resistance

  connectToWiFi();       // Connect to WiFi network

  client.setServer(mqttServer, mqttPort);  // Setup MQTT server and port
  client.setCallback(mqttCallback);         // Register MQTT callback handler

  lcd.clear();
  lcd.print("Ready");

  randomSeed(analogRead(0));  // Seed random generator for simulated vitals
}

// ---------- Loop ----------
void loop() {
  if (!client.connected()) {
    connectToMQTT();  // Reconnect to MQTT broker if disconnected
  }
  client.loop();      // Process incoming MQTT messages and maintain connection

  // Read and publish sensor data at defined intervals
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();
    readAndPublishSensors();
  }

  // Clear command display on LCD after timeout period
  if (lastCommand != "" && (millis() - lastCommandTime > commandDisplayTime)) {
    lastCommand = "";
    lcd.clear();
  }
}

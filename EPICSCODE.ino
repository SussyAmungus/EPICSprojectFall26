#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DFRobot_STCC4.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// --- Wi-Fi Settings ---
const char* ssid = "NETGEAR51_2GEXT";
const char* password = "largehat027";
const char* pc_ip = "172.16.0.33"; // Find your computer's IPv4 address
const int udp_port = 4210; //do not change

WiFiUDP udp;

// --- Pin Definitions (UNCHANGED) ---
const int VDD_EN_PIN = 15;      // PMOS gate: LOW = ON, HIGH = OFF
const int MOISTURE_PROBE_1 = 34; // Analog input 1
const int MOISTURE_PROBE_2 = 35; // Analog input 2 
const int ONE_WIRE_BUS = 2;     // DS18B20 Data pin
const int I2C_SDA = 21;          
const int I2C_SCL = 22;          

// --- Sensor Objects ---
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
DFRobot_STCC4_I2C stcc4(&Wire, 0x64); 

// --- STCC4 Compensation Constants ---
const uint16_t tCompensation = 26;
const uint16_t hCompensation = 55;
const uint16_t pCompensation = 950;

    


void setup() {
  Serial.begin(115200);

Serial.print("Connecting to Wi-Fi");



WiFi.begin(ssid, password);


while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println("\nWi-Fi Connected!");

  while (!Serial) { delay(10); }

  // 1. Power up the sensors via PMOS
  pinMode(VDD_EN_PIN, OUTPUT);
  digitalWrite(VDD_EN_PIN, LOW);
  Serial.println("System: Sensors Powering Up...");
  delay(1000); 

  // 2. Initialize DS18B20 Temp Sensor
  ds18b20.begin();

  // 3. Initialize I2C and the STCC4 CO2 Sensor
  Wire.begin(I2C_SDA, I2C_SCL); 
  
  while (!stcc4.begin()) {
    Serial.println("Error: STCC4 not found. Check wiring to SDA 21 and SCL 22!");
    delay(1000);
  }
  Serial.println("STCC4 Initialized Successfully.");

  // 4. Apply Working Compensations
  stcc4.setRHTcompensation(tCompensation, hCompensation);
  stcc4.setPressureCompensation(pCompensation);
}

void loop() {
  // --- 1. Read Soil Moisture ---
  analogRead(MOISTURE_PROBE_1); 
  delay(10); 
  int moisture1 = analogRead(MOISTURE_PROBE_1); 

  analogRead(MOISTURE_PROBE_2); 
  delay(10);
  int moisture2 = analogRead(MOISTURE_PROBE_2); 

  int moistureDifference = moisture1 - moisture2;

  // --- 2. Read DS18B20 Temperature ---
  ds18b20.requestTemperatures();
  float soilTempC = ds18b20.getTempCByIndex(0);

  // --- 3. Read STCC4 CO2 Sensor (Single-Shot Method) ---
  static uint16_t co2Concentration = 0;
  static float ambientTemp = 0.0;
  static float ambientHumidity = 0.0;
  uint16_t sensorStatus = 0;

  stcc4.wakeup();
  delay(100);

  stcc4.singleMeasurement();
  delay(900); // Wait for the physical measurement to finish

  // Overwrite the variables if data is ready
  if (stcc4.measurement(&co2Concentration, &ambientTemp, &ambientHumidity, &sensorStatus)) {
    // Variables are now successfully updated
  }

  // Immediately put the STCC4 back to sleep to save power and prevent PMOS brownouts
  stcc4.sleep();


// Construct a single comma-separated string of all your data
  String payload = "Probe1:" + String(moisture1) + 
                   ",Probe2:" + String(moisture2) + 
                   ",Grad:" + String(moistureDifference) + 
                   ",SoilC:" + String(soilTempC) + 
                   ",AmbC:" + String(ambientTemp) + 
                   ",AmbHum:" + String(ambientHumidity) + 
                   ",CO2:" + String(co2Concentration);

// Send it over Wi-Fi
udp.beginPacket(pc_ip, udp_port);
udp.print(payload);
udp.endPacket();

// You can also print it locally to check it via USB
Serial.println("Sent: " + payload);

  // Wait ~9 seconds to maintain the 10-second duty cycle from the working demo
  // This prevents the STCC4's micro-heater from overheating or drawing too much continuous current
  delay(19000); 
}
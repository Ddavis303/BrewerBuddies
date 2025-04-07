#include "Particle.h"
#include "OneWire.h"

// Photon 2 pin assignments
const int16_t dsVCC = D6;  // Power pin
const int16_t dsData = D7; // Data pin
const int16_t dsGND = D8;  // Ground pin

// Initialize OneWire directly
OneWire oneWire(dsData);

// Device address
uint8_t deviceAddress[8];
bool deviceFound = false;

// Function prototypes
void readTemperature();
bool initializeSensor();
uint8_t crc8(uint8_t *addr, uint8_t len);
bool resetOneWire();
bool readBit();
void writeBit(uint8_t bit);
void writeByte(uint8_t data);
uint8_t readByte();
void releaseBus();

void setup() {
  Serial.begin(115200);
  while(!Serial.isConnected()) {
    delay(100);
  }
  
  Serial.println("\n=== DS18B20 Initialization ===");
  Serial.printf("VCC Pin: %d, Data Pin: %d, GND Pin: %d\n", dsVCC, dsData, dsGND);
  
  // Configure pins for Photon 2
  pinMode(dsGND, OUTPUT);
  digitalWrite(dsGND, LOW);
  pinMode(dsVCC, OUTPUT);
  digitalWrite(dsVCC, HIGH);
  pinMode(dsData, INPUT);  // Start as input
  
  // Give more time for power to stabilize
  Serial.println("Waiting for power to stabilize...");
  delay(3000);
  
  // Check voltage levels
  Serial.println("\nChecking voltage levels:");
  int vccLevel = digitalRead(dsVCC);
  int gndLevel = digitalRead(dsGND);
  int dataLevel = digitalRead(dsData);
  
  Serial.printf("VCC: %d, GND: %d, Data: %d\n", vccLevel, gndLevel, dataLevel);
  
  if (vccLevel != HIGH || gndLevel != LOW) {
    Serial.println("ERROR: Power configuration issue!");
    Serial.println("VCC should be HIGH and GND should be LOW");
    return;
  }
  
  // Try to initialize the sensor
  if (initializeSensor()) {
    deviceFound = true;
  } else {
    deviceFound = false;
  }
}

void releaseBus() {
  pinMode(dsData, INPUT);
  delayMicroseconds(5);
}

bool resetOneWire() {
  pinMode(dsData, OUTPUT);
  digitalWrite(dsData, LOW);
  delayMicroseconds(480);
  releaseBus();
  delayMicroseconds(70);
  return (digitalRead(dsData) == LOW);
}

bool readBit() {
  pinMode(dsData, OUTPUT);
  digitalWrite(dsData, LOW);
  delayMicroseconds(1);
  releaseBus();
  delayMicroseconds(15);
  bool bit = digitalRead(dsData);
  delayMicroseconds(45);
  return bit;
}

void writeBit(uint8_t bit) {
  pinMode(dsData, OUTPUT);
  digitalWrite(dsData, LOW);
  delayMicroseconds(bit ? 6 : 60);
  releaseBus();
  delayMicroseconds(64);
}

void writeByte(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    writeBit(data & 0x01);
    data >>= 1;
  }
  releaseBus();
}

uint8_t readByte() {
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    data >>= 1;
    if (readBit()) {
      data |= 0x80;
      Serial.print("1");
    } else {
      Serial.print("0");
    }
  }
  Serial.println();
  releaseBus();
  return data;
}

bool initializeSensor() {
  Serial.println("\nAttempting to initialize sensor...");
  
  if (resetOneWire()) {
    Serial.println("Presence pulse detected!");
    
    // Skip ROM command
    Serial.println("Sending Skip ROM command (0xCC)...");
    writeByte(0xCC);
    delayMicroseconds(100);
    
    // Start temperature conversion
    Serial.println("Sending Convert T command (0x44)...");
    writeByte(0x44);
    
    // Wait for conversion
    Serial.println("Waiting for conversion...");
    delay(750);
    
    // Reset and skip ROM again
    Serial.println("Resetting bus...");
    resetOneWire();
    
    Serial.println("Sending Skip ROM command (0xCC)...");
    writeByte(0xCC);
    delayMicroseconds(100);
    
    // Read scratchpad
    Serial.println("Sending Read Scratchpad command (0xBE)...");
    writeByte(0xBE);
    delayMicroseconds(100);
    
    // Read all 9 bytes of scratchpad
    uint8_t data[9];
    Serial.println("Reading scratchpad data (bit by bit):");
    for(int i = 0; i < 9; i++) {
      Serial.printf("Byte %d: ", i);
      data[i] = readByte();
      Serial.printf(" = 0x%02X\n", data[i]);
      delayMicroseconds(100);
    }
    
    // Print raw data
    Serial.println("Raw scratchpad data:");
    for(int i = 0; i < 9; i++) {
      Serial.printf("%02X ", data[i]);
    }
    Serial.println();
    
    // Calculate temperature (LSB is first byte)
    int16_t rawTemp = (data[1] << 8) | data[0];
    float tempC = (float)rawTemp / 16.0;
    float tempF = tempC * 9.0 / 5.0 + 32.0;
    
    Serial.printlnf("Temperature: %f°F (%f°C)", tempF, tempC);
    Serial.printf("Raw temperature value: 0x%04X\n", rawTemp);
    
    // Verify CRC
    uint8_t crc = crc8(data, 8);
    if (crc == data[8]) {
      Serial.println("CRC check passed!");
      return true;
    } else {
      Serial.printf("CRC check failed! Calculated: 0x%02X, Received: 0x%02X\n", crc, data[8]);
      return false;
    }
  } else {
    Serial.println("No presence pulse detected!");
  }
  
  Serial.println("\nTroubleshooting steps:");
  Serial.println("1. Check if 4.7kΩ pull-up resistor is connected between D7 and VCC");
  Serial.println("2. Measure voltage between VCC and GND - should be 3.3V");
  Serial.println("3. Check all connections are secure");
  Serial.println("4. Try a different data pin");
  Serial.println("5. Verify sensor is not damaged");
  Serial.println("6. Try power cycling the Photon 2");
  Serial.println("7. Check if the sensor is getting warm (indicating power)");
  return false;
}

// CRC8 calculation function
uint8_t crc8(uint8_t *addr, uint8_t len) {
  uint8_t crc = 0;
  while (len--) {
    uint8_t inbyte = *addr++;
    for (uint8_t i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

void loop() {
  if (deviceFound) {
    readTemperature();
  } else {
    Serial.println("\nTrying to detect device again...");
    if (initializeSensor()) {
      deviceFound = true;
    }
  }
  delay(2000);
}

void readTemperature() {
  Serial.println("Starting temperature reading...");
  
  if (resetOneWire()) {
    // Skip ROM command
    Serial.println("Sending Skip ROM command (0xCC)...");
    writeByte(0xCC);
    delayMicroseconds(100);
    
    // Start temperature conversion
    Serial.println("Sending Convert T command (0x44)...");
    writeByte(0x44);
    
    // Wait for conversion
    Serial.println("Waiting for conversion...");
    delay(750);
    
    // Reset and skip ROM again
    Serial.println("Resetting bus...");
    resetOneWire();
    
    Serial.println("Sending Skip ROM command (0xCC)...");
    writeByte(0xCC);
    delayMicroseconds(100);
    
    // Read scratchpad
    Serial.println("Sending Read Scratchpad command (0xBE)...");
    writeByte(0xBE);
    delayMicroseconds(100);
    
    // Read all 9 bytes of scratchpad
    uint8_t data[9];
    Serial.println("Reading scratchpad data (bit by bit):");
    for(int i = 0; i < 9; i++) {
      Serial.printf("Byte %d: ", i);
      data[i] = readByte();
      Serial.printf(" = 0x%02X\n", data[i]);
      delayMicroseconds(100);
    }
    
    // Print raw data
    Serial.println("Raw scratchpad data:");
    for(int i = 0; i < 9; i++) {
      Serial.printf("%02X ", data[i]);
    }
    Serial.println();
    
    // Calculate temperature (LSB is first byte)
    int16_t rawTemp = (data[1] << 8) | data[0];
    float tempC = (float)rawTemp / 16.0;
    float tempF = tempC * 9.0 / 5.0 + 32.0;
    
    Serial.printlnf("Temperature: %f°F (%f°C)", tempF, tempC);
    Serial.printf("Raw temperature value: 0x%04X\n", rawTemp);
    
    // Verify CRC
    uint8_t crc = crc8(data, 8);
    if (crc == data[8]) {
      Serial.println("CRC check passed!");
    } else {
      Serial.printf("CRC check failed! Calculated: 0x%02X, Received: 0x%02X\n", crc, data[8]);
    }
  } else {
    Serial.println("Failed to reset OneWire bus!");
  }
}
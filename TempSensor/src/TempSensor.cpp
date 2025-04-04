#include "Particle.h"
#include "DS18B20.h"
#include "OneWire.h"

const int MAXRETRY = 4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH = 10000;

const int16_t dsVCC = D3;
const int16_t dsData = D2;
const int16_t dsGND = D4;

void getTemp();
void publishData();
// Sets Pin D3 as data pin and the only sensor on bus
DS18B20 ds18b20(dsData, true);

char szInfo[64];
double celsius;
double fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample;

SYSTEM_MODE(SEMI_AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

void setup()
{
  Serial.begin(115200);

  pinMode(dsGND, OUTPUT);
  digitalWrite(dsGND, LOW);
  pinMode(dsVCC, OUTPUT);
  digitalWrite(dsVCC, HIGH);

  // Add initialization check
  Serial.println("Initializing sensor...");

  delay(1000);
}

void loop()
{
  if (millis() - msLastSample >= msSAMPLE_INTERVAL)
  {
    getTemp();
  }

  if (millis() - msLastMetric >= msMETRIC_PUBLISH)
  {
    Serial.println("Publishing now.");
    publishData();
  }
}

void publishData()
{
  sprintf(szInfo, "%2.2f", fahrenheit);
  Particle.publish("dsTmp", szInfo, PRIVATE);
  msLastMetric = millis();
}

void getTemp()
{
  float _temp;
  int i = 0;

  Serial.println("Starting temperature reading...");

  // Longer delay to ensure sensor is ready
  delay(2000);

  do
  {
    _temp = ds18b20.getTemperature();
    Serial.printlnf("Attempt %d: Raw temperature value: %f", i + 1, _temp);
    Serial.printlnf("CRC Check: %s", ds18b20.crcCheck() ? "PASS" : "FAIL");

    // Print the actual value before conversion
    Serial.printlnf("Raw value before conversion: %f", _temp);

    if (!ds18b20.crcCheck())
    {
      Serial.println("Waiting 100ms before next attempt...");
      delay(100);
    }
  } while (!ds18b20.crcCheck() && MAXRETRY > i++);

  if (i < MAXRETRY)
  {
    // Manual temperature conversion
    if (!isnan(_temp) && _temp != 0.0)
    { // Also check for zero
      celsius = _temp;
      fahrenheit = celsius * 9.0 / 5.0 + 32.0;
      Serial.printlnf("Success! Temperature: %f°F (%f°C)", fahrenheit, celsius);
    }
    else
    {
      celsius = fahrenheit = NAN;
      Serial.println("ERROR: Invalid temperature reading");
      Serial.println("Raw value was invalid or zero");
    }
  }
  else
  {
    celsius = fahrenheit = NAN;
    Serial.println("ERROR: Invalid reading after maximum retries");
    Serial.println("Please check:");
    Serial.println("1. 4.7kΩ pull-up resistor between D0 and VCC");
    Serial.println("2. All connections are secure");
    Serial.println("3. Sensor is properly powered (3.3V)");
  }
  msLastSample = millis();
}
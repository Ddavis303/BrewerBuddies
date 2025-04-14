/* 
 * Project Brew Buddy
 * Author: Victor Roman & Dillon Davis
 * Date: 4/10/25  
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "HX711.h"
#include "DS18B20.h"
#include "OneWire.h"
//#include "Adafruit_SSD1306.h"
//#include "Adafruit_GFX.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"
#include "Adafruit_VL53L0X.h"
#include "SPI.h"
#include "Adafruit_ILI9341.h"
#include "logo.h"

TCPClient theClient;
Adafruit_MQTT_SPARK mqtt(&theClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

//Variables
const int MAXRETRY = 4;
const uint32_t msSAMPLE_INTERVAL = 2500;
const uint32_t msMETRIC_PUBLISH = 10000;
uint32_t lastTime = 0;

const int16_t dsVCC = D3;
const int16_t dsData = D2;
//const int16_t dsGND = D4;

char szInfo[64];
double celsius;
double fahrenheit;
uint32_t msLastMetric;
uint32_t msLastSample;
const int PHPIN = A1;
float pH;
const int OLED_RESET = -1;
float acidVoltage = 2070;
float neutralVoltage = 1580;
float voltage, slope, intercept, ph = 25;

const float CALFACTOR=5.45;
const int SAMPLES=10;
float weight, rawData, calibration, liquidWeight;
int offset;

const float H20DENSITY = 1;//g/cm^3
const float RADIUS = 10.5;//cm
float volume, height, density, specificGravity;

const int TFT_DC = D5;
const int TFT_CS = D4;


//const float PI = 3.14159265358979323846;
//Functions
void MQTT_connect();
bool MQTT_ping();
void getTemp();
void publishData();
float getPH();

// Objects
DS18B20 ds18b20(dsData, true);
//TCPClient theClient;
// Adafruit_MQTT_SPARK mqtt(&theClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
//Adafruit_SSD1306 display(OLED_RESET);
HX711 myScale(A5, SCK);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
//Publish's
Adafruit_MQTT_Publish tempPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");
Adafruit_MQTT_Publish phPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/ph");
Adafruit_MQTT_Publish specificGPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/specificg");

SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

void setup()
{
  Serial.begin(9600);
  delay(3000);
  WiFi.clearCredentials();
  WiFi.setCredentials("CODA-1B30", "2512070C3758");
  //WiFi.setCredentials("DDCIOT", "ddcIOT2020");
  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Serial.printf("\n\n");
  
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);
  tft.drawBitmap(0, 0, logo, 320, 240, ILI9341_GREEN);
  //delay(5000);
  //pinMode(dsGND, OUTPUT);
  //digitalWrite(dsGND, LOW);
  pinMode(dsVCC, OUTPUT);
  digitalWrite(dsVCC, HIGH);
  pinMode(PHPIN, INPUT);
  
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // display.display();
  // display.clearDisplay();
  // display.display();

  // Add initialization check
  Serial.println("Initializing sensor...");

  //Scale Setup
  myScale.set_scale();//Initalize loadcell
  delay(6000);//let loadcell settle
 
  myScale.tare();//set the tare weight
  myScale.set_scale(CALFACTOR);//adjust when calibrating scale to desired units

  delay(1000);

  //ToF Setup
  Serial.println("Adafruit VL53L0X test");
  // if (!lox.begin()) {
  //   Serial.println(F("Failed to boot VL53L0X"));
  //   while(1);
  // }
}

void loop()
{
  //MQTT Connect
  MQTT_connect();
  MQTT_ping();

  //ToF
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
  } else {
    Serial.println(" out of range ");
  }

  //Scale
  weight = myScale.get_units(SAMPLES);
  Serial.printf("Weight: %f\n", weight);
  // liquidWeight = weight - 643;
  // Serial.printf("Liquid Weight: %f\n", liquidWeight);
  delay(2000);
  
  rawData = myScale.get_value(SAMPLES);
  offset = myScale.get_offset();
  calibration = myScale.get_scale();

  //Specific Gravity
  height = 24 - (measure.RangeMilliMeter/10);
  volume = PI * pow(RADIUS,2) * height;
  Serial.printf("Height: %f\n", height);
  Serial.printf("Volume: %f\n", volume);
  density = weight / volume;
  Serial.printf("Density: %f\n", density);
  specificGravity = density / H20DENSITY;
  Serial.printf("Specific Gravity: %f\n", specificGravity);

  //Check Temperature
  if (millis() - msLastSample >= msSAMPLE_INTERVAL)
  {
    getTemp();
  }

  if (millis() - msLastMetric >= msMETRIC_PUBLISH)
  {
    Serial.println("Publishing now.");
    publishData();
  }

  //Check pH
  //if (millis() - msLastSample >= msSAMPLE_INTERVAL)
  //{
    pH = getPH();
  //}
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(4);
  tft.printf("  Brew Buddy\n");
  tft.setTextSize(3);
  tft.printf("\nTemp: %0.2fF\n", fahrenheit);
  tft.printf("pH: %0.2f\n", pH);
  tft.printf("SG: %0.5f\n", specificGravity);
  tft.printf("Weight: %0.2f\n", weight);
  tft.printf("Distance: %0.2f\n", measure.RangeMilliMeter/10);

  //publish every 6 seconds
  if((millis()-lastTime > 6000)) {
    if(mqtt.Update()) {
        tempPub.publish(fahrenheit);
        Serial.printf("Publishing %0.2f \n",fahrenheit);
        phPub.publish(pH);
        Serial.printf("Publishing %0.2f \n",pH);
        specificGPub.publish(specificGravity);
        Serial.printf("Publishing %0.2f \n",specificGravity); 
      } 
    lastTime = millis();
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

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}

float getPH(){
  
  voltage = analogRead(PHPIN)/4095.0*3300; //Read the voltage
  slope = (7.0-4.0)/((neutralVoltage-1500)/3.0 - (acidVoltage-1500)/3.0); 
  intercept = 7.0 - slope*(neutralVoltage-1500)/3.0;
  ph = slope*(voltage-1500)/3.0+intercept; // y = mx+b
  Serial.printf("\nVoltage: %0.4f", voltage/1000.0);
  Serial.printf("\npH: %0.4f", ph);
  // display.setTextSize(2);
  // display.setTextColor(WHITE);
  // display.setCursor(0,0);
  // display.clearDisplay();
  // display.printf("\nV: %0.4f", voltage/1000.0);
  // display.printf("\npH: %0.4f", ph);
  // display.display();
  return ph;
}
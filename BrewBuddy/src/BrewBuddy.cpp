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
uint32_t lastHour = 0;

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
float gallons, volume, height, density, specificGravity;

const int TFT_DC = D5;
const int TFT_CS = D4;

// int col[8] = {tft.color565(155, 0, 50),
//               tft.color565(170, 30, 80),
//               tft.color565(195, 60, 110),
//               tft.color565(215, 90, 140),
//               tft.color565(230, 120, 170),
//               tft.color565(250, 150, 200),
//               tft.color565(255, 180, 220),
//               tft.color565(255, 210, 240)
//             };


//const float PI = 3.14159265358979323846;
//Functions
void MQTT_connect();
bool MQTT_ping();
void getTemp();
void publishData();
float getPH();
//void showmsgXY(int x, int y, int sz, const char *msg);

// Objects
DS18B20 ds18b20(dsData, true);
HX711 myScale(D15, D14);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
//Publish's
Adafruit_MQTT_Publish tempPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");
Adafruit_MQTT_Publish phPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/ph");
Adafruit_MQTT_Publish specificGPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/specificg");
Adafruit_MQTT_Publish gallonPub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/barrels");

SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

void setup()
{
  Serial.begin(9600);
  //delay(3000);
  WiFi.clearCredentials();
  WiFi.setCredentials("CODA-1B30", "2512070C3758");
  //WiFi.setCredentials("DDCIOT", "ddcIOT2020");
  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Serial.printf("\n\n");
  
  //ToF Setup
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  //Screen Setup
   tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);
  //Show Logo
  tft.drawBitmap(0, 0, logo, 320, 240, ILI9341_GREEN);
  delay(3000);
  tft.fillScreen(ILI9341_BLACK);


  pinMode(dsVCC, OUTPUT);
  digitalWrite(dsVCC, HIGH);
  pinMode(PHPIN, INPUT);
  
  // Add initialization check
  Serial.println("Initializing sensor...");

  //Scale Setup
  myScale.set_scale();//Initalize loadcell
  delay(6000);//let loadcell settle
 
  myScale.tare();//set the tare weight
  myScale.set_scale(CALFACTOR);//adjust when calibrating scale to desired units
  delay(1000);

  
}

void loop()
{
  //MQTT Connect
  MQTT_connect();
  MQTT_ping();

  //ToF
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("\nReading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    Serial.print("\nDistance (mm): "); Serial.println(measure.RangeMilliMeter);
  } else {
    Serial.println(" out of range ");
  }

  //Scale
  weight = myScale.get_units(SAMPLES);
  gallons = 3785.0 / weight;
  Serial.printf("Weight: %f\n", weight);
  // liquidWeight = weight - 643;
  // Serial.printf("Liquid Weight: %f\n", liquidWeight);
  delay(2000);
  
  // rawData = myScale.get_value(SAMPLES);
  // offset = myScale.get_offset();
  // calibration = myScale.get_scale();

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


  // //Check pH
  
   pH = getPH();


  //Print to tft
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
  tft.printf("Distance: %0.2f\n", height);
  tft.printf("Gallons: %0.2f\n", gallons);

  //publish to Adafruit every minute
  if((millis()-lastTime > 60000)) {
    if(mqtt.Update()) {
        tempPub.publish(fahrenheit);
        Serial.printf("Publishing %0.2f \n",fahrenheit);
      } 
    lastTime = millis();
  }

  //Publish to Adafruit every hour
  if((millis()-lastHour) > 3600000){
    phPub.publish(pH);
    Serial.printf("Publishing %0.2f \n",pH);
    specificGPub.publish(specificGravity);
    Serial.printf("Publishing %0.2f \n",specificGravity); 
    gallonPub.publish(gallons);
    Serial.printf("Publishing %0.2f \n",gallons);
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

// void showmsgXY(int x, int y, int sz, const char *msg)
// {
//   int16_t x1, y1;
//   uint16_t wid, ht;


//   tft.setFont(SansSerifBold9pt7b);
//   tft.setCursor(x, y);
//   tft.setTextColor(0x0000);
//   tft.setTextSize(sz);
//   tft.print(msg);
// }
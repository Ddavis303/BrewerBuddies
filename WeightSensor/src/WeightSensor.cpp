/* 
 * Project Scale
 * Author: Dillon Davis
 * Date: 3/17/25
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "HX711.h"
//#include <Adafruit_MQTT.h>
// #include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
// #include "Adafruit_MQTT/Adafruit_MQTT.h"
//#include "credentials.h"


// /************ Global State (you don't need to change this!) ***   ***************/ 
// TCPClient TheClient; 

// // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
// Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 
// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);
HX711 myScale(A5, SCK);

//Adafruit_MQTT_Publish pubFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Weight");
// void MQTT_connect();
// bool MQTT_ping();

const float CALFACTOR=5.45;
const int SAMPLES=10;

float weight, rawData, calibration;
int offset;
unsigned int lastTime;

// setup() runs once, when the device is first turned on
void setup() {
  Serial.begin(9600);
  //Serial.printf("Serial connected!\n");
  myScale.set_scale();//Initalize loadcell
  delay(6000);//let loadcell settle
 
  myScale.tare();//set the tare weight
  myScale.set_scale(CALFACTOR);//adjust when calibrating scale to desired units
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // MQTT_connect();
  // MQTT_ping();
  weight = myScale.get_units(SAMPLES);
  Serial.printf("Weight: %f\n", weight);
  delay(2000);
  
  rawData = myScale.get_value(SAMPLES);
  offset = myScale.get_offset();
  calibration = myScale.get_scale();

  // if((millis()-lastTime > 6000)) {
  //   if(mqtt.Update()) {
  //     pubFeed.publish(weight);
  //     Serial.printf("Publishing %0.2f \n",weight); 
  //     } 
  //   lastTime = millis();
  // }
}

// void MQTT_connect() {
//   int8_t ret;
 
//   // Return if already connected.
//   if (mqtt.connected()) {
//     return;
//   }
 
//   Serial.print("Connecting to MQTT... ");
 
//   while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
//        Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
//        Serial.printf("Retrying MQTT connection in 5 seconds...\n");
//        mqtt.disconnect();
//        delay(5000);  // wait 5 seconds and try again
//   }
//   Serial.printf("MQTT Connected!\n");
// }

// bool MQTT_ping() {
//   static unsigned int last;
//   bool pingStatus;

//   if ((millis()-last)>120000) {
//       Serial.printf("Pinging MQTT \n");
//       pingStatus = mqtt.ping();
//       if(!pingStatus) {
//         Serial.printf("Disconnecting \n");
//         mqtt.disconnect();
//       }
//       last = millis();
//   }
//   return pingStatus;
// }

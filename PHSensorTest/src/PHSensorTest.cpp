/* 
 * Project Operation PH
 * Author: Dillon Davis & Victor Roman
 * Date: 4/4/25
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"


const int PHPIN = A1;
float acidVoltage = 2070;
float neutralVoltage = 1580;
float voltage, slope, intercept, ph = 25;
const int OLED_RESET = -1;
Adafruit_SSD1306 display(OLED_RESET);
// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// setup() runs once, when the device is first turned on
void setup() {
  // Put initialization like pinMode and begin functions here4
  Serial.begin(9600);
  pinMode(PHPIN, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();
  display.display();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  voltage = analogRead(PHPIN)/4095.0*3300; //Read the voltage
  slope = (7.0-4.0)/((neutralVoltage-1500)/3.0 - (acidVoltage-1500)/3.0); 
  intercept = 7.0 - slope*(neutralVoltage-1500)/3.0;
  ph = slope*(voltage-1500)/3.0+intercept; // y = mx+b
  Serial.printf("\nVoltage: %0.4f", voltage/1000.0);
  Serial.printf("\npH: %0.4f", ph);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.printf("\nV: %0.4f", voltage/1000.0);
  display.printf("\npH: %0.4f", ph);
  display.display();
}

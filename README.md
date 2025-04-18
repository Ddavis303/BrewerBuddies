# Brew Buddy – IoT Brewing Analytics System
## Overview
The Brew Buddy is an IoT-based system designed to help brewers—especially small craft and home brewers—monitor essential brewing parameters in real time. Using a combination of sensors and an Argon microcontroller, the system collects temperature, pH, specific gravity (via weight), and volume (via distance), displays the data on a web dashboard, and sends text alerts when any readings fall outside predefined thresholds.
This project was developed as a capstone for an IoT Rapid Prototyping course and serves as a working proof-of-concept for a scalable tech solution in the brewing industry.
---
## Features
- Live data readings from brewing vessels
- Remote monitoring via dashboard
- SMS notifications when values exceed thresholds
- Simulated homebrew setup for testing and demonstration
---
## Sensors & Components
- *Argon* – Microcontroller
- **DS18B20** – Digital temperature sensor
- **pH Sensor** – Analog pH probe
- **Load Cell + HX711** – For measuring weight/specific gravity
- **VL53L0X TOF Sensor** – For estimating volume via liquid height
- **Breadboard**, **jumper wires**, **power supply**
- **Twilio** or similar service for SMS notifications
- **Cloud service** (e.g., Firebase, Blynk, or custom backend)
---
## How It Works
1. Sensors are connected to the Argon and calibrated for the brewing environment.
2. Readings are taken at regular intervals and sent to the cloud.
3. A dashboard displays live data for each sensor.
4. If a reading moves outside acceptable parameters (e.g., temperature too high), an SMS alert is triggered.
---
## Setup Instructions
1. **Assemble the hardware**
Connect each sensor to the Argon as specified in the wiring diagram (included below or in the gallery).
2. **Flash the Argon*
Upload the code using Arduino IDE or PlatformIO. Install required libraries:
- `OneWire`
- `DallasTemperature`
- `HX711`
- `VL53L0X`
- Any network/messaging libraries needed
3. **Configure the backend**
- Set up a Zappier for email and text alerts
- Link your Argon code to your backend using API keys
4. **Calibrate sensors**
Run test readings and calibrate the sensors based on your brewing environment.
5. **Monitor & Brew!**
Watch your live dashboard and relax—alerts will notify you of any problems.
---
## Future Improvements
- Batch tracking and historical analytics
- Predictive alerts based on trend data
- Integration with commercial brewing systems
- Mobile app interface
---
## Credits
**Built by:**
- Dillon Davis
- Victor Roman
*Capstone Team – IoT Rapid Prototyping, April 2025*

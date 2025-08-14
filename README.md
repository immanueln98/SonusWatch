# SonusWatch

**Noise Monitoring Device for Cape Town Environmental Monitoring**

SonusWatch is an ESP32-based IoT device designed to monitor and track noise levels across different suburbs in Cape Town. The system combines GPS positioning with cellular connectivity to create a distributed environmental monitoring network.

## 🎯 Project Overview

This project aims to create a comprehensive noise monitoring solution that can be deployed across Cape Town to collect real-time acoustic data, providing valuable insights into urban noise pollution patterns.

## 🛠️ Hardware Components

- **ESP32-S3** microcontroller
- **A7670E 4G/LTE** cellular modem (SIM7600 compatible)
- **GNSS/GPS** module for location tracking
- **INMP441 I2S Digital Microphone** (planned integration)
- Power management system for field deployment

## 📡 Current Implementation Status

### ✅ Completed Features

**GNSS & Cellular Connectivity (`GNSS_overMQTT.ino`)**
- GPS/GNSS positioning with TinyGPS++ library
- 4G/LTE cellular connectivity using TinyGSM
- MQTT data transmission to cloud services
- Real-time location tracking and reporting
- WiFi fallback connectivity
- JSON data formatting for sensor readings
- Power management (Normal/Standby/Power-down modes)

**Core System Functions:**
- Cellular modem initialization and AT command interface
- GPS data parsing and validation
- MQTT publish/subscribe functionality
- Error handling and connection recovery
- Configurable update intervals (GPS: 1s, MQTT: 5s)

### 🚧 In Development
- Audio capture and noise level measurement
- Integration of INMP441 I2S microphone
- Sound processing algorithms for dB measurement
- Data fusion between GPS and audio data

## 📊 Data Output

The system currently transmits GPS data in JSON format via MQTT:
```json
{
  "lat": -33.925839,
  "lng": 18.423218,
  "alt": 42.3,
  "speed": 0.0,
  "course": 287.2,
  "satellites": 8,
  "hdop": 1.2,
  "timestamp": 1234567890,
  "datetime": "2024-01-15 14:30:22"
}
```

## 🔧 Configuration

- **MQTT Broker:** Waveshare Cloud (mqtt.waveshare.cloud)
- **Update Intervals:** GPS (1s), MQTT transmission (5s)
- **Cellular Network:** Auto-detection of APN settings
- **Serial Communication:** 115200 baud rate

## 🚀 Getting Started

1. Install required libraries:
   - TinyGsmClient
   - TinyGPS++
   - PubSubClient
   - ArduinoJson

2. Configure your cellular APN and MQTT credentials
3. Upload `GNSS_overMQTT.ino` to your ESP32
4. Monitor via Serial console at 115200 baud

## 📍 Deployment Target

Initial deployment focused on Cape Town suburbs for comprehensive urban noise mapping and environmental monitoring.

## 🔜 Roadmap

- [ ] Integrate INMP441 microphone for noise measurement
- [ ] Implement real-time dB level calculation
- [ ] Add data logging and local storage
- [ ] Develop web dashboard for data visualization
- [ ] Deploy sensor network across target locations


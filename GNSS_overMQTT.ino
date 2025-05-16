// Define the modem model
#define TINY_GSM_MODEM_SIM7600  // Compatible with A7670
#define TINY_GSM_USE_GPRS true

// Serial configuration - USING SINGLE SERIAL PORT FOR BOTH AT COMMANDS AND GPS
#define SerialMon Serial
#define RXD2 17   
#define TXD2 18    
#define powerPin 4 
#define STASSID "SVITAR"
#define STAPSK "L1l1anj13!!"

// Include necessary libraries
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// Initialize single serial for modem
HardwareSerial cellSerial(1); // Use UART1 for all communication with A7670E

// Module state management
enum ModuleState {
  STATE_GSM_COMMAND,  // Using AT commands for GSM
  STATE_GPS_DATA      // Reading GPS NMEA data
};
ModuleState currentState = STATE_GSM_COMMAND;
unsigned long stateChangeTime = 0;
const unsigned long STATE_SWITCH_DELAY = 1000; // 1 second delay when switching states

// Initialize objects
const char *ssid = STASSID;
const char *password = STAPSK;
TinyGsm modem(cellSerial);
TinyGsmClient gsmClient(modem);
WiFiClient espClient;
PubSubClient mqttClient(espClient); // Default to WiFi, will switch if needed
TinyGPSPlus gps;

// Network credentials
const char apn[] = "";  // Leave blank for auto-detection
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT Configuration
const char* mqtt_server = "mqtt.waveshare.cloud";
const int mqtt_port = 1883;
const char* mqtt_client_id = "f66194fa";
const char* mqtt_topic = "Pub/955/37/f66194fa";
const char* sub = "Sub/955/37/f66194fa";

// JSON buffer for sending data
StaticJsonDocument<256> gpsJson;

// Timing variables
unsigned long lastGPSUpdate = 0;
unsigned long lastMQTTSend = 0;
const unsigned long GPS_UPDATE_INTERVAL = 1000;  // Read GPS every 1 second
const unsigned long MQTT_SEND_INTERVAL = 5000;   // Send to MQTT every 5 seconds
bool useGSM = false;  // Flag to determine which connection method to use

// Function to switch to AT command mode
void switchToATMode() {
  if (currentState == STATE_GPS_DATA) {
    SerialMon.println("Switching to AT command mode");
    // Clear any pending data
    while(cellSerial.available()) cellSerial.read();
    currentState = STATE_GSM_COMMAND;
    stateChangeTime = millis();
  }
}

// Function to switch to GPS data mode
void switchToGPSMode() {
  if (currentState == STATE_GSM_COMMAND) {
    SerialMon.println("Switching to GPS data mode");
    // Enable NMEA output if not already enabled
    modem.sendAT("+CGNSSPORTSWITCH=0,1");
    modem.waitResponse(3000);
    // Clear any pending AT command responses
    while(cellSerial.available()) cellSerial.read();
    currentState = STATE_GPS_DATA;
    stateChangeTime = millis();
  }
}

bool initModem() {
  SerialMon.println("Initializing modem...");
  
  // Configure Serial for modem
  cellSerial.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(3000);
  
  // Ensure we're in AT command mode
  currentState = STATE_GSM_COMMAND;
  
  // Test AT communication
  SerialMon.println("Testing modem communication...");
  modem.sendAT("");
  if (!modem.waitResponse(1000)) {
    SerialMon.println("Failed to communicate with modem");
    return false;
  }
  
  // Initialize modem
  if (!modem.init()) {
    SerialMon.println("Failed to initialize modem");
    return false;
  }
  
  // Get modem info
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  
  return true;
}

bool connectGPRS() {
  // Make sure we're in AT command mode
  switchToATMode();
  
  // Wait for state switch delay
  if (millis() - stateChangeTime < STATE_SWITCH_DELAY) {
    delay(STATE_SWITCH_DELAY);
  }
  
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  
  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }
  
  // Connect to GPRS
  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  
  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
    useGSM = true;
    String ccid = modem.getSimCCID();
    SerialMon.print("CCID: ");
    SerialMon.println(ccid);
  }
  
  return true;
}

bool initGPS() {
  SerialMon.println("Initializing GPS...");
  
  // Make sure we're in AT command mode
  switchToATMode();
  
  // Wait for state switch delay
  if (millis() - stateChangeTime < STATE_SWITCH_DELAY) {
    delay(STATE_SWITCH_DELAY);
  }
  
  // Power on GPS
  modem.sendAT("+CGNSSPWR=1");
  if (!modem.waitResponse(3000)) {
    SerialMon.println("Failed to power on GPS");
    return false;
  }
  delay(1000);
  
  // Configure GPS output port
  modem.sendAT("+CGNSSPORTSWITCH=0,1");
  if (!modem.waitResponse(3000)) {
    SerialMon.println("Failed to configure GPS port");
    return false;
  }
  
  // Start GPS output
  modem.sendAT("+CGNSSTST=1");
  if (!modem.waitResponse(3000)) {
    SerialMon.println("Failed to start GPS output");
    return false;
  }
  
  SerialMon.println("GPS initialized");
  
  // Now switch to GPS mode to start reading NMEA data
  switchToGPSMode();
  
  return true;
}

void readGPS() {
  // Make sure we're in GPS data mode
  if (currentState != STATE_GPS_DATA) {
    switchToGPSMode();
    return; // Wait for next loop iteration
  }
  
  // Read GPS data
  while (cellSerial.available() > 0) {
    char c = cellSerial.read();
    gps.encode(c);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("] ");
  for (int i = 0; i < length; i++) {
    SerialMon.print((char)payload[i]);
  }
  SerialMon.println();
}

void reconnect() {
  // Switch to appropriate client
  if (useGSM && modem.isGprsConnected()) {
    mqttClient.setClient(gsmClient);
  } else {
    mqttClient.setClient(espClient);
  }
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
  while (!mqttClient.connected()) {
    Serial.print("MQTT not connected.. Trying to connect");

    if (mqttClient.connect(mqtt_client_id)) {
      Serial.println("Client connected");
      mqttClient.subscribe(sub);
    } else {
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void sendGPSData() {
  // Temporarily switch to AT command mode if needed for MQTT operations
  if (useGSM && currentState == STATE_GPS_DATA) {
    switchToATMode();
    delay(100); // Small delay to ensure switch
  }
  
  if (gps.location.isValid()) {
    // Prepare JSON data
    gpsJson.clear();
    gpsJson["lat"] = gps.location.lat();
    gpsJson["lng"] = gps.location.lng();
    gpsJson["alt"] = gps.altitude.meters();
    gpsJson["speed"] = gps.speed.kmph();
    gpsJson["course"] = gps.course.deg();
    gpsJson["satellites"] = gps.satellites.value();
    gpsJson["hdop"] = gps.hdop.hdop();
    gpsJson["timestamp"] = millis();

    // Add date and time if available
    if (gps.date.isValid() && gps.time.isValid()) {
      char datetime[32];
      sprintf(datetime, "%04d-%02d-%02d %02d:%02d:%02d",
              gps.date.year(), gps.date.month(), gps.date.day(),
              gps.time.hour(), gps.time.minute(), gps.time.second());
      gpsJson["datetime"] = datetime;
    }
    
    // Serialize JSON
    char buffer[256];
    size_t n = serializeJson(gpsJson, buffer);
    
    // Publish to MQTT
    if (mqttClient.publish(mqtt_topic, buffer, n)) {
      SerialMon.println("GPS data sent");
    } else {
      SerialMon.println("Failed to send GPS data");
    }
  }
  
  // Switch back to GPS mode after sending
  if (useGSM) {
    switchToGPSMode();
  }
}

void printGPSInfo() {
  SerialMon.print("GPS: ");
  SerialMon.print("Sats=");
  SerialMon.print(gps.satellites.value());
  SerialMon.print(" HDOP=");
  SerialMon.print(gps.hdop.hdop());
  
  if (gps.location.isValid()) {
    SerialMon.print(" Lat=");
    SerialMon.print(gps.location.lat(), 6);
    SerialMon.print(" Lng=");
    SerialMon.print(gps.location.lng(), 6);
    SerialMon.print(" Alt=");
    SerialMon.print(gps.altitude.meters());
  } else {
    SerialMon.print(" Location: INVALID");
  }
  SerialMon.println();
}

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // Initialize serial monitor
  SerialMon.begin(115200);
  delay(10);
  SerialMon.println("ESP32 GSM GPS MQTT Tracker");
  
  // Initialize modem
  if (!initModem()) {
    SerialMon.println("Modem initialization failed!");
    while (1) delay(1000);
  }
  
  // Try WiFi first
  // setup_wifi();
  
  // As fallback, try GSM
  if (WiFi.status() != WL_CONNECTED) {
    // Connect to cellular network
    if (!connectGPRS()) {
      SerialMon.println("GPRS connection failed!");
      // Continue anyway, will retry in loop
    } else {
      useGSM = true;
    }
  }
  
  // Initialize GPS
  if (!initGPS()) {
    SerialMon.println("GPS initialization failed!");
    while (1) delay(1000);
  }
  
  // Connect to MQTT broker
  reconnect();
  
  SerialMon.println("Setup complete!");
}

void loop() {
  // Check connection status
  if (useGSM) {
    // If using GSM, check GPRS connection
    if (!modem.isGprsConnected()) {
      SerialMon.println("GPRS disconnected! Reconnecting...");
      useGSM = false; // Reset flag
      
      // Try WiFi instead
      if (WiFi.status() != WL_CONNECTED) {
        setup_wifi();
      }
      
      // If WiFi still not available, try GSM again
      if (WiFi.status() != WL_CONNECTED) {
        if (connectGPRS()) {
          useGSM = true;
        } else {
          delay(10000);
          return;
        }
      }
    }
  } else {
    // If using WiFi, check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
      SerialMon.println("WiFi disconnected! Reconnecting...");
      setup_wifi();
      
      // If WiFi reconnection fails, try GSM
      if (WiFi.status() != WL_CONNECTED && !useGSM) {
        if (connectGPRS()) {
          useGSM = true;
        }
      }
    }
  }
  
  // Check MQTT connection
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  
  // Read GPS data
  if (millis() - lastGPSUpdate >= GPS_UPDATE_INTERVAL) {
    readGPS();
    printGPSInfo();
    lastGPSUpdate = millis();
  }
  
  // Send GPS data via MQTT
  if (millis() - lastMQTTSend >= MQTT_SEND_INTERVAL) {
    sendGPSData();
    lastMQTTSend = millis();
  }
  
  // Small delay to prevent watchdog issues
  delay(100);
}
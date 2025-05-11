// Define the modem model
#define TINY_GSM_MODEM_SIM7600  // Compatible with A7670
#define TINY_GSM_USE_GPRS true

// Serial configuration
#define SerialAT Serial1
#define SerialMon Serial
#define RXD2 17   
#define TXD2 18    
#define powerPin 4 
#define STASSID "SAVITAR"
#define STAPSK "L1l1anj13!!"

// Include necessary libraries
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <WiFi.h>

// Initialize objects
const char *ssid = STASSID;
const char *password = STAPSK;
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);
WiFiClient espClient;
// PubSubClient mqttClient(gsmClient);
PubSubClient mqttClient(espClient);
TinyGPSPlus gps;

// Network credentials
const char apn[] = "";  // Leave blank for auto-detection
const char gprsUser[] = "";
const char gprsPass[] = "";

// HiveMQ Cloud Configuration
const char* mqtt_server = "mqtt.waveshare.cloud";  // test.mosquito
const int mqtt_port = 1883;  // TLS port
const char* mqtt_client_id = "f66194fa";
const char* mqtt_topic = "Pub/955/37/f66194fa";
const char* sub = "Sub/955/37/f66194fa";
const char* mqtt_user = ""; 
const char* mqtt_pass = "";

// HiveMQ Cloud Root CA Certificate (ISRG Root X1)
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
"-----END CERTIFICATE-----\n";
// GPS Configuration
HardwareSerial gpsSerial(2);  // Use UART2 for GPS data
const uint32_t GPSBaud = 9600;

// JSON buffer for sending data
StaticJsonDocument<256> gpsJson;

// Timing variables
unsigned long lastGPSUpdate = 0;
unsigned long lastMQTTSend = 0;
const unsigned long GPS_UPDATE_INTERVAL = 1000;  // Read GPS every 1 second
const unsigned long MQTT_SEND_INTERVAL = 5000;   // Send to MQTT every 5 seconds

bool initModem() {
  SerialMon.println("Initializing modem...");
  
  // Configure Serial for modem
  SerialAT.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(3000);
  
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
    String ccid = modem.getSimCCID();
    SerialMon.print("CCID: ");
    SerialMon.println(ccid);
  }
  
  return true;
}

//GPS Initializaiton and Control Funtions
bool initGPS() {
  SerialMon.println("Initializing GPS...");
  
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
  
  // Initialize GPS serial
  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXD2, TXD2);
  
  SerialMon.println("GPS initialized");
  return true;
}

void readGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

//MQTT FUNCTIONS
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("] ");
  for (int i = 0; i < length; i++) {
    SerialMon.print((char)payload[i]);
  }
  SerialMon.println();
}

bool mqttConnect() {
  SerialMon.print("Connecting to MQTT...");

    // Set SSL/TLS options
  // gsmClient.setCACert(root_ca);
  
  // Configure MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  return true;
  
  
  // SerialMon.print("Server Connected...");
  
  // SerialMon.print("Callback set...");

  // return false;
  
  // Connect to MQTT broker
  // if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
  //   SerialMon.println(" success");
  //   // Subscribe to topics if needed
  //   // mqttClient.subscribe("gps/commands");
  //   return true;
  // } else {
  //   SerialMon.print(" failed, rc=");
  //   SerialMon.println(mqttClient.state());
  //   return false;
  // }
}
void reconnect() {
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
  
  // Connect to cellular network
  // if (!connectGPRS()) {
  //   SerialMon.println("GPRS connection failed!");
  //   while (1) delay(1000);
  // }
  
  // Initialize GPS
  if (!initGPS()) {
    SerialMon.println("GPS initialization failed!");
    while (1) delay(1000);
  }
  //WIFI setup

  setup_wifi();
  // Connect to MQTT broker
  if (!mqttConnect()) {
    SerialMon.println("MQTT connection failed!");
    // Continue anyway, will retry in loop
  }
  
  SerialMon.println("Setup complete!");
}

void loop() {
  // Check GPRS connection
  // if (!modem.isGprsConnected()) {
  //   SerialMon.println("GPRS disconnected! Reconnecting...");
  //   if (!connectGPRS()) {
  //     delay(10000);
  //     return;
  //   }
  // }
  
  // Check MQTT connection
  if (!mqttClient.connected()) {
    // if (!mqttConnect()) {
    //   delay(5000);
    //   return;
    // }
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


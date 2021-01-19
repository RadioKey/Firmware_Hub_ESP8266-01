#include <configurator.h>
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#define DEBUG true

/**
 * This access point credentials used to start hub in access point mode 
 * and configure connection of hab as station to access point
 */
#define SECRET_WIFI_AUTOCONFIGURE_SSID "RadioKey"
#define SECRET_WIFI_AUTOCONFIGURE_PASSWORD "zGatQd12la"

#define DEVICE_ID "HUBESP01"
#define PROTOCOL_VERSION "0.0.1"

int TRANSMITTER_PIN = 1;

/**
 * For ESP8266 interrupt equals pin
 */
int RECEIVER_INTERRUPT = 2;

bool shouldSaveConfiguration = false;

configuration conf;

// See https://github.com/sui77/rc-switch/ for details
RCSwitch switcher = RCSwitch();

// See https://github.com/knolleary/pubsubclient
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String MACAddress;

const char* subscribeMqttTopic; // Unique topic per hub defined as MAC address of device
const char* publishMqttTopic = "response";

String mqttClientId;

void debug(const char* message)
{
  #if defined(DEBUG) && DEBUG == true
  Serial.println(message);
  #endif
}

void sendMQTTHeartbeat()
{
  DynamicJsonDocument doc(1024);
  doc["command"] = "heartbeat";
  doc["mac"] = MACAddress;
  doc["deviceId"] = DEVICE_ID;
  doc["protocolVersion"] = PROTOCOL_VERSION;

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish(
    publishMqttTopic, 
    payload.c_str()
  );

  debug("Heartbeat sent");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  debug("MQTT callback called");

  char* charPayload = new char[length];
  for (unsigned int i = 0; i < length; i++) {
    charPayload[i] = (char)payload[i];
  }

  DynamicJsonDocument request(2048);
  deserializeJson(request, charPayload);

  const char* command = request["command"];

  if (strcmp(command, "send") == 0) {
    debug("Sending code command received");

    // send received code to switcher
    int protocol = (int)request["protocol"];
    int repeats = (int)request["repeats"];
    int pulseLength = (int)request["pulseLength"];
    const char* code = request["code"];
    
    switcher.enableTransmit(TRANSMITTER_PIN);
    switcher.setProtocol(protocol);
    switcher.setPulseLength(pulseLength);
    switcher.setRepeatTransmit(repeats);
    switcher.send(code);
  } else if (strcmp(command, "scan") == 0) {
    debug("Scanning code command received");
    switcher.enableReceive(RECEIVER_INTERRUPT);
    if (switcher.available()) {
      debug("Code found");
      DynamicJsonDocument response(1024);
      response["command"] = "scanResult";
      response["macAddress"] = MACAddress;
      response["value"] = (String)switcher.getReceivedValue();
      response["bitLength"] = (String)switcher.getReceivedBitlength();
      response["protocol"] = (String)switcher.getReceivedProtocol();
      response["delay"] = (String)switcher.getReceivedDelay();
      //response["raw"] = switcher.getReceivedRawdata();

      String payload;
      serializeJson(response, payload);

      mqttClient.publish(
        publishMqttTopic, 
        payload.c_str()
      );

      switcher.resetAvailable();
    }
  } else if (strcmp(command, "reconfigure") == 0) {
    debug("Forget configuration command received");
    WiFi.disconnect();
    delay(2000);
    ESP.restart();
  } else if (strcmp(command, "restart") == 0) {
    debug("Restart command received");
    ESP.restart();
  }

  // send heartbeat
  sendMQTTHeartbeat();

  // sleep for next command read
  delay(5000);
}

void connectMqttClient(const char* user, const char* password) {
  debug("Starting to connect to MQTT server with credentials:");
  debug(user);
  debug(password);

  while (!mqttClient.connected()) {
    // Attempt to connect
    debug("Try to connect to MQTT server");

    if (mqttClient.connect(mqttClientId.c_str(), user, password)) {
      debug("Connected to MQTT successfully");

      // heartbeat
      sendMQTTHeartbeat();

      // subscribe to commands
      debug("Subscribe to MQTT topic");
      mqttClient.subscribe(subscribeMqttTopic);
    } else {
      debug("Can not connect to MQTT server, try restart...");
      delay(5000);
    }
  }
}

void wifiManagerSaveConfigCallback() {
  shouldSaveConfiguration = true;
}

void setup() {
  Serial.begin(115200);

  MACAddress = WiFi.macAddress();
  mqttClientId = "RadioKey_" + MACAddress;

  // configure application
  debug("Loading configuration");
  conf = loadConfiguration();

  // Wifi management
  WiFiManager wifiManager;
  debug("Obtaining WiFi credentials");

  # if defined(DEBUG) && DEBUG == true
  wifiManager.setDebugOutput(true);
  # endif

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(wifiManagerSaveConfigCallback);

  // Configure custom params during wifi management
  WiFiManagerParameter mqttHostParameter("mqtt_host", "MQTT host", conf.mqttHost, 40);
  wifiManager.addParameter(&mqttHostParameter);

  WiFiManagerParameter mqttPortParameter("mqtt_port", "MQTT port", conf.mqttPort, 6);
  wifiManager.addParameter(&mqttPortParameter);

  WiFiManagerParameter mqttUserParameter("mqtt_user", "MQTT user", conf.mqttUser, 40);
  wifiManager.addParameter(&mqttUserParameter);

  WiFiManagerParameter mqttPasswordParameter("mqtt_password", "MQTT password", conf.mqttPassword, 40);
  wifiManager.addParameter(&mqttPasswordParameter);

  // connect wifi
  wifiManager.autoConnect(
    SECRET_WIFI_AUTOCONFIGURE_SSID, 
    SECRET_WIFI_AUTOCONFIGURE_PASSWORD
  );

  debug("WiFi configured");

  // renew params from config
  if (shouldSaveConfiguration) {
    debug("Storing configuration");
    strcpy(conf.mqttHost, mqttHostParameter.getValue());
    strcpy(conf.mqttPort, mqttPortParameter.getValue());
    strcpy(conf.mqttUser, mqttUserParameter.getValue());
    strcpy(conf.mqttPassword, mqttPasswordParameter.getValue());

    storeConfiguration(conf);

    shouldSaveConfiguration = false;
  }

  // if configuration not exists, force
  if (strcmp(conf.mqttHost, "") == 0) {
    debug("Empty configuration after successfull wifi connection, restart configuration");
    wifiManager.resetSettings();
    delay(1000);
    ESP.restart();
  }

  // MQTT client
  debug("Configuring MQTT server host and port:");
  debug(conf.mqttHost);
  debug(conf.mqttPort);

  mqttClient.setServer(
    conf.mqttHost, 
    (__uint16_t)atoi(conf.mqttPort)
  );

  mqttClient.setCallback(mqttCallback);

  subscribeMqttTopic = MACAddress.c_str();
}

void loop() {  
  if (!mqttClient.connected()) {
    connectMqttClient(conf.mqttUser, conf.mqttPassword);
  }

  mqttClient.loop();
}

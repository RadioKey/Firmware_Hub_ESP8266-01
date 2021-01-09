#include <configurator.h>
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

/**
 * This access point credentials used to start hub in access point mode 
 * and configure connection of hab as station to access point
 */
#define SECRET_WIFI_AUTOCONFIGURE_SSID "RadioKey"
#define SECRET_WIFI_AUTOCONFIGURE_PASSWORD "zGatQd12la"

#define PROTOCOL_VERSION "0.0.1"

int TRANSMITTER_PIN = 1;
int RECEIVER_PIN = 2;

bool shouldSaveConfiguration = false;

configuration conf;

// See https://github.com/sui77/rc-switch/ for details
RCSwitch switcher = RCSwitch();

// See https://github.com/knolleary/pubsubclient
PubSubClient mqttClient;

String MACAddress;

const char* commandsSubscribeMqttTopic;
const char* provisionPublishMqttTopic = "provision";

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  if (topic == commandsSubscribeMqttTopic) {
    String command = doc["command"];

    if (command == "send") {
      // send received code to switcher
      int protocol = doc["protocol"];
      int repeats = doc["repeats"];
      int pulseLength = doc["pulseLength"];
      const char* code = doc["code"];

      switcher.setProtocol(protocol);
      switcher.setPulseLength(pulseLength);
      switcher.setRepeatTransmit(repeats);
      switcher.send(code);
    } else if (command == "scan") {
      
    } else if (command == "reset") {
      WiFi.disconnect();
      delay(2000);
      ESP.restart();
    } else if (command == "restart") {
      ESP.restart();
    }
  }

  // sleep for next command read
  delay(5000);
}

void reconnectMqttClient(char* user, char* password) {
  while (!mqttClient.connected()) {
    String clientId = "RadioKeyHub_" + MACAddress;

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), user, password)) {
      // subscribe to commands
      mqttClient.subscribe(commandsSubscribeMqttTopic);
      
      // provision devive
      DynamicJsonDocument doc(1024);
      doc["mac"] = MACAddress;
      doc["protocolVersion"] = PROTOCOL_VERSION;

      String payload;
      serializeJson(doc, payload);

      mqttClient.publish(
        provisionPublishMqttTopic, 
        payload.c_str()
      );
    } else {
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

  // configure application
  conf = loadConfiguration();

  // configure switcher
  switcher.enableTransmit(TRANSMITTER_PIN);

  // Wifi management
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setConfigPortalTimeout(30);
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

  // renew params from config
  if (shouldSaveConfiguration) {
    strcpy(conf.mqttHost, mqttHostParameter.getValue());
    strcpy(conf.mqttPort, mqttPortParameter.getValue());
    strcpy(conf.mqttUser, mqttUserParameter.getValue());
    strcpy(conf.mqttPassword, mqttPasswordParameter.getValue());

    storeConfiguration(conf);

    shouldSaveConfiguration = false;
  }

  // MQTT client
  mqttClient.setServer(
    conf.mqttHost, 
    atoi(conf.mqttPort)
  );

  mqttClient.setCallback(mqttCallback);

  commandsSubscribeMqttTopic = MACAddress.c_str();
}

void loop() {  
  if (!mqttClient.connected()) {
    reconnectMqttClient(conf.mqttUser, conf.mqttPassword);
  }

  mqttClient.loop();
}

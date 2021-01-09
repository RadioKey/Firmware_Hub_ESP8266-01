#include <FS.h>
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

int TRANSMITTER_PIN = 2;
char* configFilePath = "/config.json";
bool shouldSaveConfiguration = false;

// See https://github.com/sui77/rc-switch/ for details
RCSwitch switcher = RCSwitch();

// See https://github.com/knolleary/pubsubclient
PubSubClient mqttClient;

char mqttHost[40];
char mqttPort[6]  = "1883";
char mqttUser[40];
char mqttPassword[40];

String MACAddress;

const char* commandsMqttTopic;

void loadConfiguration() 
{
  if (SPIFFS.begin()) {
    if (SPIFFS.exists(configFilePath)) {
      File configFile = SPIFFS.open(configFilePath, "r");
      if (configFile) {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        deserializeJson(json, buf.get());

        strcpy(mqttHost, json["mqttHost"]);
        strcpy(mqttPort, json["mqttPort"]);
        strcpy(mqttUser, json["mqttUser"]);
        strcpy(mqttPassword, json["mqttPassword"]);
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

  if (topic == commandsMqttTopic) {
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
    }
  }

  // sleep for next command read
  delay(20000);
}

void reconnectMqttClient(char* user, char* password)
{
// Loop until we're reconnected
  while (!mqttClient.connected()) {
    String clientId = "RadioKeyHub_" + MACAddress;

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), user, password)) {
      mqttClient.subscribe(commandsMqttTopic);
    } else {
      delay(5000);
    }
  }
}

void wifiManagerSaveConfigCallback()
{
  shouldSaveConfiguration = true;
}

void setup() {
  Serial.begin(115200);

  // configure switcher
  switcher.enableTransmit(TRANSMITTER_PIN);

  // Wifi management
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setConfigPortalTimeout(30);
  wifiManager.setSaveConfigCallback(wifiManagerSaveConfigCallback);

  // Configure custom params during wifi management
  WiFiManagerParameter mqttHostParameter("mqtt_host", "MQTT host", mqttPort, 40);
  wifiManager.addParameter(&mqttHostParameter);

  WiFiManagerParameter mqttPortParameter("mqtt_port", "MQTT port", mqttPort, 6);
  wifiManager.addParameter(&mqttPortParameter);

  WiFiManagerParameter mqttUserParameter("mqtt_user", "MQTT user", mqttUser, 40);
  wifiManager.addParameter(&mqttUserParameter);

  WiFiManagerParameter mqttPasswordParameter("mqtt_password", "MQTT password", mqttPassword, 40);
  wifiManager.addParameter(&mqttPasswordParameter);

  // connect wifi
  wifiManager.autoConnect(
    SECRET_WIFI_AUTOCONFIGURE_SSID, 
    SECRET_WIFI_AUTOCONFIGURE_PASSWORD
  );

  // renew params from config
  strcpy(mqttHost, mqttHostParameter.getValue());
  strcpy(mqttPort, mqttPortParameter.getValue());
  strcpy(mqttUser, mqttUserParameter.getValue());
  strcpy(mqttPassword, mqttPasswordParameter.getValue());

  if (shouldSaveConfiguration) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqttHost"] = mqttHost;
    json["mqttPort"] = mqttPort;
    json["mqttUser"] = mqttUser;
    json["mqttPassword"] = mqttPassword;

    File configFile = SPIFFS.open(configFilePath, "w");

    json.printTo(configFile);
    configFile.close();

    shouldSaveConfiguration = false;
  }

  MACAddress = WiFi.macAddress();

  // MQTT client
  mqttClient.setServer(
    mqttHost, 
    mqttPort
  );
  mqttClient.setCallback(mqttCallback);

  commandsMqttTopic = MACAddress.c_str();
}

void loop() {  
  if (!mqttClient.connected()) {
    reconnectMqttClient(mqttUser, mqttPassword);
  }

  mqttClient.loop();
}

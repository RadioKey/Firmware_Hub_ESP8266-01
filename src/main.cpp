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
PubSubClient mqttClient;

String MACAddress;

const char* subscribeMqttTopic; // Unique topic per hub defined as MAC address of device
const char* publishMqttTopic = "response";

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
  debug("MQTT callback:");

  DynamicJsonDocument request(1024);
  deserializeJson(request, payload);

  if (topic == subscribeMqttTopic) {
    String command = request["command"];
    debug(command.c_str());

    if (command == "send") {
      // send received code to switcher
      int protocol = request["protocol"];
      int repeats = request["repeats"];
      int pulseLength = request["pulseLength"];
      const char* code = request["code"];

      debug("Sending code");
      switcher.enableTransmit(TRANSMITTER_PIN);
      switcher.setProtocol(protocol);
      switcher.setPulseLength(pulseLength);
      switcher.setRepeatTransmit(repeats);
      switcher.send(code);
    } else if (command == "scan") {
      debug("Scanning code");
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
    } else if (command == "reconfigure") {
      debug("Forget configuration");
      WiFi.disconnect();
      delay(2000);
      ESP.restart();
    } else if (command == "restart") {
      debug("Restart");
      ESP.restart();
    }
  }

  // send heartbeat
  sendMQTTHeartbeat();

  // sleep for next command read
  delay(5000);
}

void connectMqttClient(const char* user, const char* password) {
  debug("Connecting to MQTT server");

  while (!mqttClient.connected()) {
    String clientId = "RadioKeyHub_" + MACAddress;

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), user, password)) {
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

  // configure application
  debug("Loading configuration");
  conf = loadConfiguration();

  // Wifi management
  debug("Obtaining WiFi credentials");
  WiFiManager wifiManager;

  // wifiManager.resetSettings();

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

  // MQTT client
  debug("Configuring MQTT server host and port");
  mqttClient.setServer(
    conf.mqttHost, 
    atoi(conf.mqttPort)
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

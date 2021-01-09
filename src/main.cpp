#include <secrets.h>
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

int TRANSMITTER_PIN = 2;

// See https://github.com/sui77/rc-switch/ for details
RCSwitch switcher = RCSwitch();

// See https://github.com/knolleary/pubsubclient
PubSubClient mqttClient;

String MACAddress;

const char* commandsMqttTopic;

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

void reconnectMqttClient()
{
// Loop until we're reconnected
  while (!mqttClient.connected()) {
    String clientId = "RadioKeyHub_" + MACAddress;

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      mqttClient.subscribe(commandsMqttTopic);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // configure switcher
  switcher.enableTransmit(TRANSMITTER_PIN);

  // Wifi management
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setConfigPortalTimeout(30);
  wifiManager.autoConnect(
    SECRET_WIFI_AUTOCONFIGURE_SSID, 
    SECRET_WIFI_AUTOCONFIGURE_PASSWORD
  );

  MACAddress = WiFi.macAddress();

  // MQTT client
  mqttClient.setServer(
    SECRET_MQTT_BROCKER_HOST, 
    SECRET_MQTT_BROCKER_PORT
  );
  mqttClient.setCallback(mqttCallback);

  commandsMqttTopic = MACAddress.c_str();
}

void loop() {  
  if (!mqttClient.connected()) {
    reconnectMqttClient();
  }

  mqttClient.loop();
}

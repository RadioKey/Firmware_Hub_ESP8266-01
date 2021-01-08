#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <secrets.h>

int TRANSMITTER_PIN = 2;

char* commandsMqttTopicName = SECRET_MQTT_BROCKER_COMMANDS_TOPIC;

// See https://github.com/sui77/rc-switch/ for details
RCSwitch switcher = RCSwitch();

// See https://github.com/knolleary/pubsubclient
PubSubClient mqttClient;

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

  // MQTT client
  mqttClient.setServer(
    SECRET_MQTT_BROCKER_HOST, 
    SECRET_MQTT_BROCKER_PORT
  );
  mqttClient.setCallback(mqttCallback);
}

void loop() {  
  if (!mqttClient.connected()) {
    reconnectMqttClient();
  }

  mqttClient.loop();
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  // send received code to switcher
  switcher.setProtocol(1);
  switcher.setPulseLength(309);
  switcher.setRepeatTransmit(25);
  switcher.send("110111101110011000110001"); // first button
  switcher.send("110111101110011000110100"); // second button

  // sleep for next command read
  delay(20000);
}

void reconnectMqttClient()
{
// Loop until we're reconnected
  while (!mqttClient.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      mqttClient.subscribe(SECRET_MQTT_BROCKER_TOPIC);
    } else {
      delay(5000);
    }
  }
}

#include <FS.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <configurator.h>

char* configFilePath = CONFIGURATION_PATH;

configuration loadConfiguration() {
  configuration conf;

  // LittleFS.begin() will format FS if it can not be mounted, uncomment line below to foce reformat
  // LittleFS.format();
  
  if (LittleFS.begin()) {
    if (LittleFS.exists(configFilePath)) {
      File configFile = LittleFS.open(configFilePath, "r");
      if (configFile) {
        // read from file
        size_t size = configFile.size();
        char buf[size];
        configFile.readBytes(buf, size);

        Serial.write("Config: ");
        Serial.println(buf);

        // parse json
        DynamicJsonDocument json(1024);
        deserializeJson(json, buf);

        // fill struct
        strcpy(conf.mqttHost, json["mqttHost"]);
        strcpy(conf.mqttPort, json["mqttPort"]);
        strcpy(conf.mqttUser, json["mqttUser"]);
        strcpy(conf.mqttPassword, json["mqttPassword"]);
      } else {
        Serial.println("Can not open configuration file");
      }
    } else {
      Serial.println("Configuration file not exists");
    }
  } else {
    Serial.println("Begin FS error");
  }

  return conf;
}

void storeConfiguration(configuration conf) {
  DynamicJsonDocument doc(1024);
  doc["mqttHost"] = conf.mqttHost;
  doc["mqttPort"] = conf.mqttPort;
  doc["mqttUser"] = conf.mqttUser;
  doc["mqttPassword"] = conf.mqttPassword;

  File configFile = LittleFS.open(configFilePath, "w");

  serializeJson(doc, configFile);
  configFile.close();
}
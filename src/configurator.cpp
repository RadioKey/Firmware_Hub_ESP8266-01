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
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        deserializeJson(json, buf.get());

        strcpy(conf.mqttHost, json["mqttHost"]);
        strcpy(conf.mqttPort, json["mqttPort"]);
        strcpy(conf.mqttUser, json["mqttUser"]);
        strcpy(conf.mqttPassword, json["mqttPassword"]);
      }
    }
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
#include <FS.h>
#include <ArduinoJson.h>

#define CONFIGURATION_PATH "/config.json"

struct configuration {
    char mqttHost[40];
    char mqttPort[6]  = "1883";
    char mqttUser[40];
    char mqttPassword[40];
};

configuration loadConfiguration();
void storeConfiguration(configuration conf);
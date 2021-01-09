/**
 * This access point credentials used to start hub in access point mode 
 * and configure connection of hab as station to access point
 */
#define SECRET_WIFI_AUTOCONFIGURE_SSID "RadioKey"
#define SECRET_WIFI_AUTOCONFIGURE_PASSWORD "zGatQd12la"

/**
 * Use following server for test purposes
 * ./bin/artemis create radiokey --require-login --user test --password test
 * ./radiokey/bin/artemis run - start broker
 * ./radiokey/bin/artemis-service start - start broker in background
 * 
 * HTTP Server started at http://localhost:8161
 * Artemis Jolokia REST API available at http://localhost:8161/console/jolokia
 * Artemis Console available at http://localhost:8161/console
 */
#define SECRET_MQTT_BROCKER_HOST "192.168.1.2" // Ip address of MQTT brocker
#define SECRET_MQTT_BROCKER_PORT 8161
#define SECRET_MQTT_BROCKER_USER "test"
#define SECRET_MQTT_BROCKER_PASSWORD "test"
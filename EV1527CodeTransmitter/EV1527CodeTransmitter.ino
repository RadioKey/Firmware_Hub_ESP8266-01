#include <RCSwitch.h>
#include <ESP8266WiFi.h>

#define SECRET_WIFI_SSID "" // PLACE YOUR SSID HERE
#define SECRET_WIFI_PASSWORD "" // PLACE YOUR PASSWORD HERE
#define SECRET_CONTROLLER_HOST "" // PLACE YOUR CONTROLLER HOST HERE, Fot test: nc -v -l 80
#define SECRET_CONTROLLER_PORT 80

int TRANSMITTER_PIN = 2;
int LED_PIN = 1;

const char* controllerHost = SECRET_CONTROLLER_HOST;
const uint16_t controllerPort = SECRET_CONTROLLER_PORT;

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASSWORD;

// See https://github.com/sui77/rc-switch/ for details
RCSwitch switcher = RCSwitch();

void setup() {
  Serial.begin(9600);

  // configure switcher
  switcher.enableTransmit(TRANSMITTER_PIN);
  switcher.setProtocol(350);
  switcher.setPulseLength(309);
  switcher.setRepeatTransmit(25);

  // configure led
  pinMode(LED_PIN, OUTPUT);
  
  // Configure WiFi
  /* 
   * Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
   * would try to act as both a client and an access-point and could cause
   * network-issues with your other WiFi-devices on your WiFi-network. 
   */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  digitalWrite(LED_PIN, HIGH);

  // try connect controller
  WiFiClient client;
  if (!client.connect(controllerHost, controllerPort)) {
    Serial.println("connection failed");
    delay(5000);
    return;
  }

  Serial.println("Sending request to server");
  if (client.connected()) {
    client.println("GET /commands?mac= HTTP/1.1");
    client.print("Host: ");
    client.println(SECRET_CONTROLLER_HOST);
  }

  // wait for data to be available
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      delay(60000);
      return;
    }
  }

  // read code from controller
  char ch;
  while (client.available()) {
    ch = static_cast<char>(client.read());
    Serial.print(ch);
  }

  // stop connection to controller
  Serial.println();
  Serial.println("closing connection");
  client.stop();
  
  // send received code to switcher
  switcher.send("110111101110011000110001"); // first button
  switcher.send("110111101110011000110100"); // second button

  // inform about stopping of receiving process
  delay(1000);
  digitalWrite(LED_PIN, LOW);

  // sleep for next command read
  delay(20000);
}

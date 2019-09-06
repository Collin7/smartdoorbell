#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

//USER CONFIGURED SECTION START//
const char* ssid = "WIFI SSID";
const char* password = "WIFI PASSWORD";
const char* mqtt_server = "MQTT SERVER IP";
const int mqtt_port = 1883;
const char *mqtt_user = "MQTT USERNAME";
const char *mqtt_pass = "MQTT PASSWORD";
const char *mqtt_client_name = "DoorbellController"; // Client connections can't have the same connection name
//USER CONFIGURED SECTION END//

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;

// Variables
//Door bell
bool alreadyTriggered = false;
const int doorBellPin = 16; //marked as D0 on the board
const int silencePin = 15;  //marked as D4 on the board

bool boot = true;

//Topics
const char* doorbellTopic = "cmnd/doorbell/POWER";

//Functions

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void reconnect() {
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) {
    if (retries < 15) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        if (boot == true) {
          client.publish("checkIn/DoorbellMCU", "Rebooted");
          boot = false;
        }
        if (boot == false) {
          client.publish("checkIn/DoorbellMCU", "Reconnected");
        }
        // ... and resubscribe
        client.subscribe(doorbellTopic);
      }
      else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if (retries > 14) {
      ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  Serial.println(newPayload);
  Serial.println();
  if (newTopic == doorbellTopic) {
    if (newPayload == "OFF") {
      digitalWrite(silencePin, LOW);
      client.publish("stat/doorbell/POWER", "OFF", true);
    }
    if (newPayload == "ON") {
      digitalWrite(silencePin, HIGH);
      client.publish("stat/doorbell/POWER", "ON", true);
    }
  }
}

void getDoorBell() {
  if (digitalRead(doorBellPin) == 1 && alreadyTriggered == false) {
    client.publish("cmnd/pressed/POWER1", "ON");
    Serial.println("Doorbell Pressed, just published - Ding");
    alreadyTriggered = true;
    timer.setTimeout(6000, resetTrigger);
  }
}

void resetTrigger() {
  alreadyTriggered = false;
}

void checkIn() {
  client.publish("checkIn/doorbellMCU", "OK");
}

//Run once setup
void setup() {
  Serial.begin(115200);
  
  pinMode(doorBellPin, INPUT_PULLDOWN_16);
  pinMode(silencePin, OUTPUT);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ArduinoOTA.setHostname("doorbellController");
  ArduinoOTA.begin();

  timer.setInterval(120000, checkIn);
  timer.setInterval(200, getDoorBell);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  timer.run();
}

#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

//USER CONFIGURED SECTION START//
const char* ssid = "Morpheus";
const char* password = "b5eea63f65";
const char* mqtt_server = "192.168.0.3";
const int mqtt_port = 1883;
const char *mqtt_user = "CDW-SmartHouse";
const char *mqtt_pass = "!M0rpheus";
const char *mqtt_client_name = "DoorSensorController"; // Client connections can't have the same connection name
//USER CONFIGURED SECTION END//

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;

// Variables
//Door bell
bool alreadyTriggered = false;
const int doorBellPin = 16; //marked as D0 on the board
const int silencePin = 2;  //marked as D4 on the board
//Door contacts
const int frontDoorPin = 0; //marked as D3 on the board
const int garageToHouseDoorPin = 5; //marked as D1 on the board
const int patioDoorPin = 4; //marked as D2 on the board
const int kitchenDoorPin = 14; //marked as D5 on the board
const int mainGarageDoorPin = 12; //marked a6 D3 on the board
const int backGarageDoorPin = 13; //marked a7 D3 on the board

int frontDoorOldStatus = 1;
int garageToHouseDoorOldStatus = 1;
int patioDoorOldStatus = 1;
int kitchenDoorOldStatus = 1;
int mainGarageDoorOldStatus = 1;
int backGarageDoorOldStatus = 1;

bool boot = true;

//Topics
const char* doorbellTopic = "";


//Payloads

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
        client.subscribe("doorbell/commands");
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
  if (newTopic == "doorbell/commands") {
    if (newPayload == "Silent Doorbell") {
      digitalWrite(silencePin, LOW);
      client.publish("state/doorbell", "Silent Doorbell", true);
    }
    if (newPayload == "Audio Doorbell") {
      digitalWrite(silencePin, HIGH);
      client.publish("state/doorbell", "Audio Doorbell", true);
    }
  }
}

void getDoorBell() {
  if (digitalRead(doorBellPin) == 1 && alreadyTriggered == false) {
    client.publish("doorbell", "Ding");
    alreadyTriggered = true;
    timer.setTimeout(6000, resetTrigger);
  }
}

void resetTrigger() {
  alreadyTriggered = false;
}

void getFrontDoorState() {
  int newStatus = digitalRead(frontDoorPin);
  if (newStatus != frontOldStatus && newStatus == 0) {
    client.publish("cmnd/security/doors/frontdoor", "CLOSED", true);
    frontOldStatus = newStatus;
    Serial.println("Door closed is published");
  }
  if (newStatus != frontOldStatus && newStatus == 1) {
    client.publish("cmnd/security/doors/frontdoor", "OPEN", true);
    frontOldStatus = newStatus;
    Serial.println("Door open is published");
  }
}

void getGarageToHouseDoorState() {
  int newStatus = digitalRead(garageToHouseDoorPin);
  if (newStatus != garageToHouseDoorOldStatus && newStatus == 0) {
    client.publish("cmnd/security/doors/frontdoor", "CLOSED", true);
    garageToHouseDoorOldStatus = newStatus;
    Serial.println("Front Door closed is published");
  }
  if (newStatus != garageToHouseDoorOldStatus && newStatus == 1) {
    client.publish("cmnd/security/doors/frontdoor", "OPEN", true);
    garageToHouseDoorOldStatus = newStatus;
    Serial.println("Front Door open is published");
  }
}

//void getPatioDoorState() {
//  int newStatus = digitalRead(frontDoorPin);
//  if (newStatus != frontOldStatus && newStatus == 0) {
//    client.publish("cmnd/security/doors/frontdoor", "CLOSED", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door closed is published");
//  }
//  if (newStatus != frontOldStatus && newStatus == 1) {
//    client.publish("cmnd/security/doors/frontdoor", "OPEN", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door open is published");
//  }
//}
//
//void getKitchenDoorState() {
//  int newStatus = digitalRead(frontDoorPin);
//  if (newStatus != frontOldStatus && newStatus == 0) {
//    client.publish("cmnd/security/doors/frontdoor", "CLOSED", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door closed is published");
//  }
//  if (newStatus != frontOldStatus && newStatus == 1) {
//    client.publish("cmnd/security/doors/frontdoor", "OPEN", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door open is published");
//  }
//}
//
//void getMainGarageDoorState() {
//  int newStatus = digitalRead(frontDoorPin);
//  if (newStatus != frontOldStatus && newStatus == 0) {
//    client.publish("cmnd/security/doors/frontdoor", "CLOSED", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door closed is published");
//  }
//  if (newStatus != frontOldStatus && newStatus == 1) {
//    client.publish("cmnd/security/doors/frontdoor", "OPEN", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door open is published");
//  }
//}
//
//void getBackGarageDoorState() {
//  int newStatus = digitalRead(frontDoorPin);
//  if (newStatus != frontOldStatus && newStatus == 0) {
//    client.publish("cmnd/security/doors/frontdoor", "CLOSED", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door closed is published");
//  }
//  if (newStatus != frontOldStatus && newStatus == 1) {
//    client.publish("cmnd/security/doors/frontdoor", "OPEN", true);
//    frontOldStatus = newStatus;
//    Serial.println("Door open is published");
//  }
//}

void checkIn() {
  client.publish("checkIn/doorbellMCU", "OK");
}

//Run once setup
void setup() {
  Serial.begin(115200);

  // GPIO Pin Setup
  pinMode(frontDoorPin, INPUT_PULLUP);
  pinMode(doorBellPin, INPUT_PULLDOWN_16);
  pinMode(silencePin, OUTPUT);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ArduinoOTA.setHostname("doorbellMCU");
  ArduinoOTA.begin();

  timer.setInterval(120000, checkIn);
  timer.setInterval(500, getFrontDoorState);
  timer.setInterval(500, getGarageToHouseDoorState);
  timer.setInterval(500, getPatioDoorState);
  timer.setInterval(500, getKitchenDoorState);
  timer.setInterval(500, getMainGarageDoorState);
  timer.setInterval(500, getBackGarageDoorState);
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

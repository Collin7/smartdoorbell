#include <DHT.h>
#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

const char* host = "Doorbell Controller";
const char* ssid = "SSID HERE";
const char* password = "WIFI PASSWORD";
const char *mqtt_user = "MQTT USERNAME";
const char *mqtt_pass = "MQTT PASSWORD";
const int mqtt_port = 1883;

#define mqtt_server "192.168.0.3"
#define doorbellTopic "cmnd/doorbell/POWER"
#define temperature_topic "kitchen/sensor/dht/temperature"
#define humidity_topic "kitchen/sensor/dht/humidity"

#define DHTTYPE DHT22

//This can be used to output the date the code was compiled
const char compile_date[] = __DATE__ " " __TIME__;

//Door bell
bool alreadyTriggered = false;
const int doorBellPin = 16; //marked as D0 on the board
const int silencePin = 15;  //marked as D8 on the board
const int DHT22_PIN = 2;    //D4

bool boot = true;


WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
DHT dht(DHT22_PIN, DHTTYPE);

float temperature = 0;
float humidity = 0;

//Run once setup
void setup() {
  Serial.begin(115200);

  pinMode(doorBellPin, INPUT_PULLDOWN_16);
  pinMode(silencePin, OUTPUT);

  dht.begin();

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  ArduinoOTA.setHostname("Doorbell Controller");
  ArduinoOTA.begin();

  timer.setInterval(120000, checkIn);
  timer.setInterval(200, getDoorBell);

  timer.setInterval(50000, checkDHT);
  timer.setInterval(60000, publishDHT);
}

void loop() {
  //If MQTT client can't connect to broker, then reconnect
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); //the mqtt function that processes MQTT messages
  ArduinoOTA.handle();
  timer.run();
}

void checkDHT() {
  temperature = dht.readTemperature();
  delay(2100);
  humidity = dht.readHumidity();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Sample OK: ");
  Serial.print(temperature);
  Serial.print(" *C, ");
  Serial.print(humidity);
  Serial.println(" RH%");

}

void publishDHT() {
  client.publish(temperature_topic, String(temperature).c_str());
  client.publish(humidity_topic, String(humidity).c_str());
}

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
      if (client.connect(host, mqtt_user, mqtt_pass)) {
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

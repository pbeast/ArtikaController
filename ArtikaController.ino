/*
  Example for receiving
  
  https://github.com/sui77/rc-switch/
  
  If you want to visualize a telegram copy the raw data and 
  paste it into http://test.sui.li/oszi/
*/

#include <RCSwitch.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

#include <map>
#include <string>

#include "RfCode.h"
#include "WebServer.h"
#include "config.h"

// Increase MQTT buffer size for discovery messages
#define MQTT_MAX_PACKET_SIZE_FOR_DISCOVERY 768

#define RC_RECEIVE_PIN  13
#define RC_SEND_PIN     12
#define BTN_PIN         14

const char* mqttTopic = "artika-fan";
const char* mqttTopicForUpdates = "artika-fan-updates";
const char* mqttLightStateTopic = "artika/light/state";
const char* mqttFanStateTopic = "artika/fan/state";
const char* mqttFanSpeedStateTopic = "artika/fan/speed/state";
const char* deviceId = "artika_controller";

RCSwitch mySwitchReceive = RCSwitch();
RCSwitch mySwitchSend = RCSwitch();

WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);
ESP8266WebServer webServer(80);


bool skipNextReceive = false;
bool ignoreReceivedCodes = false;

unsigned int brightnessLevel = 5;
bool isLightOn = true;
bool isFanOn = false;
unsigned int currentFanSpeed = 0;

void connectToWiFi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pubSubClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
  pubSubClient.setBufferSize(MQTT_MAX_PACKET_SIZE_FOR_DISCOVERY);
  pubSubClient.setCallback(callback);
}

void setup() {
  Serial.begin(115200);

  fillCommandsMap();

  connectToWiFi();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP); //INPUT_PULLUP

  digitalWrite(LED_BUILTIN, HIGH);

  mySwitchSend.enableTransmit(RC_SEND_PIN);
  mySwitchSend.setProtocol(1);
  mySwitchSend.setPulseLength(405);

  // Sync fan state: turn off fan on startup (before enabling receiver)
  Serial.println("Syncing fan state: turning fan off");
  mySwitchSend.send(commandsToCodes["fan-off"].code, commandsToCodes["fan-off"].length);
  delay(500);

  // Now enable receiver after sync command
  mySwitchReceive.enableReceive(digitalPinToInterrupt(RC_RECEIVE_PIN));

  // Start mDNS and web server
  setupMDNS();
  setupWebServer();

  Serial.println("");
  Serial.println("----------------- STARTED -----------------");
}

void publishHomeAssistantDiscovery() {
  // Device information
  String device = "{\"identifiers\":[\"" + String(deviceId) + "\"],"
                  "\"name\":\"Artika Fan Controller\","
                  "\"model\":\"ESP8266 RF Controller\","
                  "\"manufacturer\":\"Custom\"}";

  // Light discovery
  String lightConfig = "{\"name\":\"Artika Light\","
                       "\"unique_id\":\"" + String(deviceId) + "_light\","
                       "\"command_topic\":\"artika/light/set\","
                       "\"state_topic\":\"" + String(mqttLightStateTopic) + "\","
                       "\"brightness_command_topic\":\"artika/light/brightness/set\","
                       "\"brightness_state_topic\":\"artika/light/brightness/state\","
                       "\"brightness_scale\":5,"
                       "\"payload_on\":\"ON\","
                       "\"payload_off\":\"OFF\","
                       "\"device\":" + device + "}";

  // Fan discovery with percentage speed control
  String fanConfig = "{\"name\":\"Artika Fan\","
                     "\"unique_id\":\"" + String(deviceId) + "_fan\","
                     "\"command_topic\":\"artika/fan/set\","
                     "\"state_topic\":\"" + String(mqttFanStateTopic) + "\","
                     "\"percentage_command_topic\":\"artika/fan/speed/set\","
                     "\"percentage_state_topic\":\"artika/fan/speed/state\","
                     "\"payload_on\":\"ON\","
                     "\"payload_off\":\"OFF\","
                     "\"speed_range_min\":1,"
                     "\"speed_range_max\":3,"
                     "\"device\":" + device + "}";

  Serial.print("Light config size: ");
  Serial.println(lightConfig.length());
  bool lightResult = pubSubClient.publish("homeassistant/light/artika_controller_light/config", lightConfig.c_str(), true);
  Serial.print("Published light discovery: ");
  Serial.println(lightResult ? "SUCCESS" : "FAILED");

  Serial.print("Fan config size: ");
  Serial.println(fanConfig.length());
  bool fanResult = pubSubClient.publish("homeassistant/fan/artika_controller_fan/config", fanConfig.c_str(), true);
  Serial.print("Published fan discovery: ");
  Serial.println(fanResult ? "SUCCESS" : "FAILED");


  if (lightResult && fanResult)
    digitalWrite(LED_BUILTIN, LOW);
}

void publishCurrentState() {
  // Publish light state
  String lightState = isLightOn ? "ON" : "OFF";
  pubSubClient.publish(mqttLightStateTopic, lightState.c_str());
  delay(50);

  // Publish brightness state
  pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
  delay(50);

  // Publish fan state and speed
  String fanState = isFanOn ? "ON" : "OFF";
  pubSubClient.publish(mqttFanStateTopic, fanState.c_str());
  delay(50);

  // Publish fan speed (1-3, or 0 for off)
  pubSubClient.publish(mqttFanSpeedStateTopic, String(currentFanSpeed).c_str());

  Serial.print("Published current state to HA - Light: ");
  Serial.print(lightState);
  Serial.print(", Brightness: ");
  Serial.print(brightnessLevel);
  Serial.print(", Fan: ");
  Serial.print(fanState);
  Serial.print(" (speed: ");
  Serial.print(currentFanSpeed);
  Serial.println(")");
}

void reconnectMQTT() {
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (pubSubClient.connect("ArtikaControllerClient", MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      pubSubClient.subscribe(mqttTopic);
      pubSubClient.subscribe("artika/light/set");
      pubSubClient.subscribe("artika/light/brightness/set");
      pubSubClient.subscribe("artika/fan/set");
      pubSubClient.subscribe("artika/fan/speed/set");

      publishHomeAssistantDiscovery();

      // Publish current state after reconnecting
      delay(500);
      publishCurrentState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void handleRFCommand(unsigned long receivedCode) {
  String receivedCommand = "";

  // Find command name by code
  for (auto& pair : commandsToCodes) {
    if (pair.second.code == receivedCode) {
      receivedCommand = pair.first.c_str();
      break;
    }
  }

  if (receivedCommand.length() > 0) {
    // Update state variables and publish to MQTT
    if (receivedCommand == "toggle-light") {
      isLightOn = !isLightOn;
      String lightState = isLightOn ? "ON" : "OFF";
      pubSubClient.publish(mqttLightStateTopic, lightState.c_str());
      Serial.print("Published light state: ");
      Serial.println(lightState);
    } else if (receivedCommand == "brightness-up") {
      if (brightnessLevel < 5) {
        brightnessLevel++;
        pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
      }
    } else if (receivedCommand == "brightness-down") {
      if (brightnessLevel > 1) {
        brightnessLevel--;
        pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
      }
    } else if (receivedCommand == "fan-off") {
      isFanOn = false;
      currentFanSpeed = 0;
      pubSubClient.publish(mqttFanStateTopic, "OFF");
      pubSubClient.publish(mqttFanSpeedStateTopic, "0");
      Serial.println("Published fan state: OFF (speed: 0)");
    } else if (receivedCommand == "fan-low") {
      isFanOn = true;
      currentFanSpeed = 1;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "1");
      Serial.println("Published fan state: ON (speed: 1)");
    } else if (receivedCommand == "fan-medium") {
      isFanOn = true;
      currentFanSpeed = 2;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "2");
      Serial.println("Published fan state: ON (speed: 2)");
    } else if (receivedCommand == "fan-high") {
      isFanOn = true;
      currentFanSpeed = 3;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "3");
      Serial.println("Published fan state: ON (speed: 3)");
    }
  }
}


void processRFReceive(unsigned long& lastReceivedCode, unsigned long& lastReceivedTime) {
  unsigned long receivedCode = mySwitchReceive.getReceivedValue();

  // output(receivedCode, mySwitchReceive.getReceivedBitlength(), mySwitchReceive.getReceivedDelay(), mySwitchReceive.getReceivedRawdata(), mySwitchReceive.getReceivedProtocol());

  mySwitchReceive.resetAvailable();

  // return;

  unsigned long currentTime = millis();

  // Debounce: ignore if same code received within 100ms
  if (receivedCode == lastReceivedCode && (currentTime - lastReceivedTime) < 100) {
    Serial.println("Ignored duplicate command (debounce)");
    return;
  }

  lastReceivedCode = receivedCode;
  lastReceivedTime = currentTime;

  Serial.print("Received: ");
  Serial.println(receivedCode);

  handleRFCommand(receivedCode);
}

void handleButtonPress() {
  static int buttonState = HIGH;
  static int lastButtonState = LOW;
  static unsigned long lastDebounceTime = 0;
  static const unsigned long debounceDelay = 50;

  int reading = digitalRead(BTN_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        skipNextReceive = true;

        Serial.println("Sending light toggle");
        RfCode toggleCode = commandsToCodes["toggle-light"];
        mySwitchSend.send(toggleCode.code, toggleCode.length);

        isLightOn = !isLightOn;
        String lightState = isLightOn ? "ON" : "OFF";
        Serial.print("Publishing light state: ");
        Serial.println(lightState);
        pubSubClient.publish(mqttLightStateTopic, lightState.c_str());
      }
    }
  }

  lastButtonState = reading;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }
  Serial.println(command);

  String topicStr = String(topic);

  // Handle light commands
  if (topicStr == "artika/light/set") {
    bool requestedState = (command == "ON");

    // Only toggle if the requested state is different from current state
    if (requestedState != isLightOn) {
      skipNextReceive = true;
      mySwitchSend.send(commandsToCodes["toggle-light"].code, commandsToCodes["toggle-light"].length);
      isLightOn = requestedState;

      // Publish state back to MQTT
      pubSubClient.publish(mqttLightStateTopic, command.c_str());

      Serial.print("Light toggled to: ");
      Serial.println(command);
    } else {
      Serial.print("Light already in requested state: ");
      Serial.println(command);
    }
  }
  // Handle brightness commands
  else if (topicStr == "artika/light/brightness/set") {
    int newBrightness = command.toInt();
    if (newBrightness >= 1 && newBrightness <= 5) {
      skipNextReceive = true;
      int diff = newBrightness - brightnessLevel;
      const char* brightCmd = (diff > 0) ? "brightness-up" : "brightness-down";

      for (int i = 0; i < abs(diff); i++) {
        mySwitchSend.send(commandsToCodes[brightCmd].code, commandsToCodes[brightCmd].length);
        delay(300);
      }
      brightnessLevel = newBrightness;

      pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
      Serial.print("Brightness set to: ");
      Serial.println(brightnessLevel);
    }
  }
  // Handle fan commands
  else if (topicStr == "artika/fan/set") {
    if (command == "ON") {
      skipNextReceive = true;
      mySwitchSend.send(commandsToCodes["fan-low"].code, commandsToCodes["fan-low"].length);
      isFanOn = true;
      currentFanSpeed = 1;
      pubSubClient.publish(mqttFanSpeedStateTopic, "1");
      Serial.println("Fan turned ON (speed: 1)");
    } else if (command == "OFF") {
      skipNextReceive = true;
      mySwitchSend.send(commandsToCodes["fan-off"].code, commandsToCodes["fan-off"].length);
      isFanOn = false;
      currentFanSpeed = 0;
      pubSubClient.publish(mqttFanSpeedStateTopic, "0");
      Serial.println("Fan turned OFF (speed: 0)");
    }
  }
  // Handle fan speed commands
  else if (topicStr == "artika/fan/speed/set") {
    int speed = command.toInt();
    if (speed >= 0 && speed <= 3) {
      skipNextReceive = true;
      const char* speedCmd = nullptr;

      switch (speed) {
        case 0:
          speedCmd = "fan-off";
          isFanOn = false;
          currentFanSpeed = 0;
          break;
        case 1:
          speedCmd = "fan-low";
          isFanOn = true;
          currentFanSpeed = 1;
          break;
        case 2:
          speedCmd = "fan-medium";
          isFanOn = true;
          currentFanSpeed = 2;
          break;
        case 3:
          speedCmd = "fan-high";
          isFanOn = true;
          currentFanSpeed = 3;
          break;
      }

      if (speedCmd != nullptr) {
        mySwitchSend.send(commandsToCodes[speedCmd].code, commandsToCodes[speedCmd].length);
        String fanState = isFanOn ? "ON" : "OFF";
        pubSubClient.publish(mqttFanStateTopic, fanState.c_str());
        pubSubClient.publish(mqttFanSpeedStateTopic, String(speed).c_str());
        Serial.print("Fan speed set to: ");
        Serial.print(speed);
        Serial.print(" (");
        Serial.print(fanState);
        Serial.println(")");
      }
    }
  }
  // Handle direct commands (original topic)
  else {
    std::string commandKey = command.c_str();
    auto it = commandsToCodes.find(commandKey);

    if (it != commandsToCodes.end()) {
      Serial.print("Sending command: ");
      Serial.println(commandKey.c_str());
      skipNextReceive = true;
      mySwitchSend.send(it->second.code, it->second.length);

      // Update fan state for direct commands
      if (commandKey == "fan-off") {
        isFanOn = false;
        currentFanSpeed = 0;
      } else if (commandKey == "fan-low") {
        isFanOn = true;
        currentFanSpeed = 1;
      } else if (commandKey == "fan-medium") {
        isFanOn = true;
        currentFanSpeed = 2;
      } else if (commandKey == "fan-high") {
        isFanOn = true;
        currentFanSpeed = 3;
      }
    } else {
      Serial.print("Unknown command: ");
      Serial.println(commandKey.c_str());
    }
  }
}

void loop() {
  if (!pubSubClient.connected()) {
    reconnectMQTT();
  }
  pubSubClient.loop();
  webServer.handleClient();
  MDNS.update();

  static unsigned long lastReceivedCode = 0;
  static unsigned long lastReceivedTime = 0;

  if (mySwitchReceive.available()) {
    if (ignoreReceivedCodes) {
      mySwitchReceive.resetAvailable();
    } else {
      if (!skipNextReceive) {
        processRFReceive(lastReceivedCode, lastReceivedTime);
      } else {
        skipNextReceive = false;
        mySwitchReceive.resetAvailable();
      }
    }
  }

  handleButtonPress();
}

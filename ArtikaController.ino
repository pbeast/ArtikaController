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
#include <ArduinoOTA.h>

#include <map>
#include <string>

#include "config.h"

#ifndef LED_BRIGHTNESS_LEVELS
#define LED_BRIGHTNESS_LEVELS 5
#endif

#include "RfCode.h"
#include "LogBuffer.h"

LogBuffer Log;

#include "WebServer.h"

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

unsigned int brightnessLevel = LED_BRIGHTNESS_LEVELS;
bool isLightOn = true;
bool isFanOn = false;
unsigned int currentFanSpeed = 0;

void sendRF(const char* commandName) {
  auto& rfCode = commandsToCodes[commandName];
  Log.print("RF TX: ");
  Log.print(commandName);
  Log.print(" (code: ");
  Log.print(rfCode.code);
  Log.println(")");
  mySwitchSend.send(rfCode.code, rfCode.length);
}

void connectToWiFi() {
  Log.println();
  Log.println();
  Log.print("Connecting to ");
  Log.println(WIFI_SSID);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Log.print(".");
  }

  Log.println("");
  Log.println("WiFi connected");
  Log.println("IP address: ");
  Log.println(WiFi.localIP());

  pubSubClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
  pubSubClient.setBufferSize(MQTT_MAX_PACKET_SIZE_FOR_DISCOVERY);
  pubSubClient.setCallback(callback);
}

void setupOTA() {
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("artika-fan");

  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // Disable RF receiver during OTA to prevent interference
    mySwitchReceive.disableReceive();

    Log.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Log.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Log.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Log.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Log.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Log.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Log.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Log.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Log.println("OTA ready");
}

void setup() {
  Serial.begin(115200);

  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {

  }
  Log.println("");
  Log.println("Serial ready");

  // CRITICAL: Initialize RF transmit pin FIRST to prevent spurious signals during boot
  // Must be done BEFORE WiFi connection which can take several seconds
  pinMode(RC_SEND_PIN, OUTPUT);
  digitalWrite(RC_SEND_PIN, LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  digitalWrite(LED_BUILTIN, HIGH);

  fillCommandsMap();

  connectToWiFi();

  mySwitchSend.enableTransmit(RC_SEND_PIN);
  mySwitchSend.setProtocol(1);
  mySwitchSend.setPulseLength(405);

  // Now enable receiver after sync command
  mySwitchReceive.enableReceive(digitalPinToInterrupt(RC_RECEIVE_PIN));

  // Start mDNS and web server
  setupMDNS();
  setupWebServer();

  // Setup OTA updates
  setupOTA();

  Log.println("");
  Log.println("----------------- STARTED -----------------");
}

void publishHomeAssistantDiscovery() {
  // Device information
  String device = "{\"identifiers\":[\"" + String(deviceId) + "\"],"
                  "\"name\":\"Artika Fan Controller\","
                  "\"model\":\"ESP8266 RF Controller\","
                  "\"manufacturer\":\"Custom\"}";

  // Light discovery with configurable brightness levels
  String lightConfig = "{\"name\":\"Artika Light\","
                       "\"unique_id\":\"" + String(deviceId) + "_light\","
                       "\"command_topic\":\"artika/light/set\","
                       "\"state_topic\":\"" + String(mqttLightStateTopic) + "\","
                       "\"brightness_command_topic\":\"artika/light/brightness/set\","
                       "\"brightness_state_topic\":\"artika/light/brightness/state\","
                       "\"brightness_scale\":" + String(LED_BRIGHTNESS_LEVELS) + ","
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

  Log.print("Light config size: ");
  Log.println(lightConfig.length());
  bool lightResult = pubSubClient.publish("homeassistant/light/artika_controller_light/config", lightConfig.c_str(), true);
  Log.print("Published light discovery: ");
  Log.println(lightResult ? "SUCCESS" : "FAILED");

  Log.print("Fan config size: ");
  Log.println(fanConfig.length());
  bool fanResult = pubSubClient.publish("homeassistant/fan/artika_controller_fan/config", fanConfig.c_str(), true);
  Log.print("Published fan discovery: ");
  Log.println(fanResult ? "SUCCESS" : "FAILED");



#ifndef SUPPRESS_STARTUP_FAN_SYNC
  // Sync fan state: turn off fan on startup (before enabling receiver)
  skipNextReceive = true;
  Log.println("Syncing fan state: turning fan off");
  sendRF("fan-off");
  delay(500);
#endif

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

  Log.print("Published current state to HA - Light: ");
  Log.print(lightState);
  Log.print(", Brightness: ");
  Log.print(brightnessLevel);
  Log.print(", Fan: ");
  Log.print(fanState);
  Log.print(" (speed: ");
  Log.print(currentFanSpeed);
  Log.println(")");
}

void reconnectMQTT() {
  while (!pubSubClient.connected()) {
    Log.print("Attempting MQTT connection...");
    if (pubSubClient.connect("ArtikaControllerClient", MQTT_USERNAME, MQTT_PASSWORD)) {
      Log.println("connected");
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
      Log.print("failed, rc=");
      Log.print(pubSubClient.state());
      Log.println(" try again in 5 seconds");
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
      bool published = pubSubClient.publish(mqttLightStateTopic, lightState.c_str());
      Log.print("RF Remote: Light toggled to ");
      Log.print(lightState);
      Log.print(" - MQTT publish: ");
      Log.println(published ? "SUCCESS" : "FAILED");
    } else if (receivedCommand == "brightness-up") {
      if (brightnessLevel < LED_BRIGHTNESS_LEVELS) {
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
      Log.println("Published fan state: OFF (speed: 0)");
    } else if (receivedCommand == "fan-low") {
      isFanOn = true;
      currentFanSpeed = 1;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "1");
      Log.println("Published fan state: ON (speed: 1)");
    } else if (receivedCommand == "fan-medium") {
      isFanOn = true;
      currentFanSpeed = 2;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "2");
      Log.println("Published fan state: ON (speed: 2)");
    } else if (receivedCommand == "fan-high") {
      isFanOn = true;
      currentFanSpeed = 3;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "3");
      Log.println("Published fan state: ON (speed: 3)");
    }
  }
}


void processRFReceive(unsigned long& lastReceivedCode, unsigned long& lastReceivedTime) {
  unsigned long receivedCode = mySwitchReceive.getReceivedValue();

  // output(receivedCode, mySwitchReceive.getReceivedBitlength(), mySwitchReceive.getReceivedDelay(), mySwitchReceive.getReceivedRawdata(), mySwitchReceive.getReceivedProtocol());

  mySwitchReceive.resetAvailable();

  // return;

  unsigned long currentTime = millis();

  // Debounce: ignore if same code received within 500ms
  if (receivedCode == lastReceivedCode && (currentTime - lastReceivedTime) < 400) {
    Log.println("Ignored duplicate RF command (debounce)");
    return;
  }

  lastReceivedCode = receivedCode;
  lastReceivedTime = currentTime;

  Log.print("Received: ");
  Log.println(receivedCode);

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

        sendRF("toggle-light");

        isLightOn = !isLightOn;
        String lightState = isLightOn ? "ON" : "OFF";
        Log.print("Publishing light state: ");
        Log.println(lightState);
        pubSubClient.publish(mqttLightStateTopic, lightState.c_str());
      }
    }
  }

  lastButtonState = reading;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Log.print("Message arrived [");
  Log.print(topic);
  Log.print("] ");

  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }
  Log.println(command);

  String topicStr = String(topic);

  // Handle light commands
  if (topicStr == "artika/light/set") {
    bool requestedState = (command == "ON");

    // Only toggle if the requested state is different from current state
    if (requestedState != isLightOn) {
      skipNextReceive = true;
      sendRF("toggle-light");
      isLightOn = requestedState;

      // Publish state back to MQTT
      pubSubClient.publish(mqttLightStateTopic, command.c_str());

      Log.print("Light toggled to: ");
      Log.println(command);
    } else {
      Log.print("Light already in requested state: ");
      Log.println(command);
    }
  }
  // Handle brightness commands
  else if (topicStr == "artika/light/brightness/set") {
    int newBrightness = command.toInt();
    if (newBrightness >= 1 && newBrightness <= LED_BRIGHTNESS_LEVELS) {
      // Only adjust brightness if light is ON
      if (isLightOn) {
        skipNextReceive = true;
        int diff = newBrightness - brightnessLevel;
        const char* brightCmd = (diff > 0) ? "brightness-up" : "brightness-down";

        for (int i = 0; i < abs(diff); i++) {
          sendRF(brightCmd);
          delay(300);
        }
        brightnessLevel = newBrightness;

        pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
        Log.print("Brightness set to: ");
        Log.println(brightnessLevel);
      } else {
        Log.println("Brightness change ignored - light is OFF");
      }
    }
  }
  // Handle fan commands
  else if (topicStr == "artika/fan/set") {
    if (command == "ON") {
      skipNextReceive = true;
      sendRF("fan-low");
      isFanOn = true;
      currentFanSpeed = 1;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "1");
      Log.println("Fan turned ON (speed: 1)");
    } else if (command == "OFF") {
      skipNextReceive = true;
      sendRF("fan-off");
      isFanOn = false;
      currentFanSpeed = 0;
      pubSubClient.publish(mqttFanStateTopic, "OFF");
      pubSubClient.publish(mqttFanSpeedStateTopic, "0");
      Log.println("Fan turned OFF (speed: 0)");
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
        sendRF(speedCmd);
        String fanState = isFanOn ? "ON" : "OFF";
        pubSubClient.publish(mqttFanStateTopic, fanState.c_str());
        pubSubClient.publish(mqttFanSpeedStateTopic, String(speed).c_str());
        Log.print("Fan speed set to: ");
        Log.print(speed);
        Log.print(" (");
        Log.print(fanState);
        Log.println(")");
      }
    }
  }
  // Handle direct commands (original topic)
  else {
    std::string commandKey = command.c_str();
    auto it = commandsToCodes.find(commandKey);

    if (it != commandsToCodes.end()) {
      skipNextReceive = true;
      sendRF(commandKey.c_str());

      // Update state and publish to MQTT for direct commands
      if (commandKey == "toggle-light") {
        isLightOn = !isLightOn;
        pubSubClient.publish(mqttLightStateTopic, isLightOn ? "ON" : "OFF");
      } else if (commandKey == "brightness-up" && brightnessLevel < LED_BRIGHTNESS_LEVELS) {
        brightnessLevel++;
        pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
      } else if (commandKey == "brightness-down" && brightnessLevel > 1) {
        brightnessLevel--;
        pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
      } else if (commandKey == "fan-off") {
        isFanOn = false;
        currentFanSpeed = 0;
        pubSubClient.publish(mqttFanStateTopic, "OFF");
        pubSubClient.publish(mqttFanSpeedStateTopic, "0");
      } else if (commandKey == "fan-low") {
        isFanOn = true;
        currentFanSpeed = 1;
        pubSubClient.publish(mqttFanStateTopic, "ON");
        pubSubClient.publish(mqttFanSpeedStateTopic, "1");
      } else if (commandKey == "fan-medium") {
        isFanOn = true;
        currentFanSpeed = 2;
        pubSubClient.publish(mqttFanStateTopic, "ON");
        pubSubClient.publish(mqttFanSpeedStateTopic, "2");
      } else if (commandKey == "fan-high") {
        isFanOn = true;
        currentFanSpeed = 3;
        pubSubClient.publish(mqttFanStateTopic, "ON");
        pubSubClient.publish(mqttFanSpeedStateTopic, "3");
      }
    } else {
      Log.print("Unknown command: ");
      Log.println(commandKey.c_str());
    }
  }
}

void loop() {
  ArduinoOTA.handle();

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
        Log.println("Skipped RF receive (echo prevention)");
        skipNextReceive = false;
        mySwitchReceive.resetAvailable();
      }
    }
  }

  handleButtonPress();
}

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <map>
#include <string>
#include "RfCode.h"

extern ESP8266WebServer webServer;
extern PubSubClient pubSubClient;

extern unsigned int brightnessLevel;
extern bool isLightOn;
extern bool isFanOn;
extern unsigned int currentFanSpeed;
extern bool skipNextReceive;

extern const char* mqttLightStateTopic;
extern const char* mqttFanStateTopic;
extern const char* mqttFanSpeedStateTopic;

extern std::map<std::string, RfCode> commandsToCodes;
extern RCSwitch mySwitchSend;

extern void calibration();

void handleWebRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".status { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".status-item { margin: 8px 0; font-size: 16px; }";
  html += ".control-section { margin: 20px 0; padding: 15px; background: #f5f5f5; border-radius: 5px; }";
  html += ".control-section h2 { margin-top: 0; color: #555; font-size: 18px; }";
  html += "button { background: #2196F3; color: white; border: none; padding: 12px 24px; margin: 5px; border-radius: 5px; cursor: pointer; font-size: 14px; }";
  html += "button:hover { background: #1976D2; }";
  html += "button.danger { background: #f44336; }";
  html += "button.danger:hover { background: #d32f2f; }";
  html += "button.success { background: #4CAF50; }";
  html += "button.success:hover { background: #45a049; }";
  html += ".btn-group { display: flex; flex-wrap: wrap; gap: 5px; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Artika Fan Controller</h1>";

  html += "<div class='status'>";
  html += "<div class='status-item'><strong>Light:</strong> " + String(isLightOn ? "ON" : "OFF") + "</div>";
  html += "<div class='status-item'><strong>Brightness:</strong> " + String(brightnessLevel) + "/5</div>";
  html += "<div class='status-item'><strong>Fan:</strong> " + String(isFanOn ? "ON" : "OFF") + "</div>";
  html += "<div class='status-item'><strong>Fan Speed:</strong> ";
  switch(currentFanSpeed) {
    case 0: html += "OFF"; break;
    case 1: html += "LOW"; break;
    case 2: html += "MEDIUM"; break;
    case 3: html += "HIGH"; break;
  }
  html += "</div></div>";

  html += "<div class='control-section'>";
  html += "<h2>Light Control</h2>";
  html += "<div class='btn-group'>";
  html += "<button onclick=\"location.href='/light/toggle'\">Toggle Light</button>";
  html += "<button onclick=\"location.href='/brightness/up'\">Brightness +</button>";
  html += "<button onclick=\"location.href='/brightness/down'\">Brightness -</button>";
  html += "</div></div>";

  html += "<div class='control-section'>";
  html += "<h2>Fan Control</h2>";
  html += "<div class='btn-group'>";
  html += "<button onclick=\"location.href='/fan/off'\">Fan Off</button>";
  html += "<button onclick=\"location.href='/fan/low'\">Low</button>";
  html += "<button onclick=\"location.href='/fan/medium'\">Medium</button>";
  html += "<button onclick=\"location.href='/fan/high'\">High</button>";
  html += "</div></div>";

  html += "<div class='control-section'>";
  html += "<h2>State Sync</h2>";
  html += "<p style='font-size: 14px; color: #666;'>If light state is out of sync, click to set:</p>";
  html += "<div class='btn-group'>";
  html += "<button class='success' onclick=\"location.href='/sync/light/on'\">Light is ON</button>";
  html += "<button onclick=\"location.href='/sync/light/off'\">Light is OFF</button>";
  html += "</div></div>";

  html += "<div class='control-section'>";
  html += "<h2>System</h2>";
  html += "<div class='btn-group'>";
  html += "<button class='danger' onclick=\"if(confirm('Run calibration?')) location.href='/calibrate'\">Run Calibration</button>";
  html += "</div></div>";

  html += "<div style='text-align: center; margin-top: 20px; font-size: 12px; color: #999;'>";
  html += "IP: " + WiFi.localIP().toString() + "<br>";
  html += "Hostname: artika-fan.local</div>";
  html += "</div></body></html>";

  webServer.send(200, "text/html", html);
}

void handleCommand(String command) {
  skipNextReceive = true;

  if (commandsToCodes.find(command.c_str()) != commandsToCodes.end()) {
    mySwitchSend.send(commandsToCodes[command.c_str()].code, commandsToCodes[command.c_str()].length);

    // Update state
    if (command == "toggle-light") {
      isLightOn = !isLightOn;
      pubSubClient.publish(mqttLightStateTopic, isLightOn ? "ON" : "OFF");
    } else if (command == "brightness-up" && brightnessLevel < 5) {
      brightnessLevel++;
      pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
    } else if (command == "brightness-down" && brightnessLevel > 1) {
      brightnessLevel--;
      pubSubClient.publish("artika/light/brightness/state", String(brightnessLevel).c_str());
    } else if (command == "fan-off") {
      isFanOn = false;
      currentFanSpeed = 0;
      pubSubClient.publish(mqttFanStateTopic, "OFF");
      pubSubClient.publish(mqttFanSpeedStateTopic, "0");
    } else if (command == "fan-low") {
      isFanOn = true;
      currentFanSpeed = 1;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "1");
    } else if (command == "fan-medium") {
      isFanOn = true;
      currentFanSpeed = 2;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "2");
    } else if (command == "fan-high") {
      isFanOn = true;
      currentFanSpeed = 3;
      pubSubClient.publish(mqttFanStateTopic, "ON");
      pubSubClient.publish(mqttFanSpeedStateTopic, "3");
    }
  }

  webServer.sendHeader("Location", "/");
  webServer.send(303);
}

void setupWebServer() {
  webServer.on("/", handleWebRoot);

  webServer.on("/light/toggle", []() { handleCommand("toggle-light"); });
  webServer.on("/brightness/up", []() { handleCommand("brightness-up"); });
  webServer.on("/brightness/down", []() { handleCommand("brightness-down"); });

  webServer.on("/fan/off", []() { handleCommand("fan-off"); });
  webServer.on("/fan/low", []() { handleCommand("fan-low"); });
  webServer.on("/fan/medium", []() { handleCommand("fan-medium"); });
  webServer.on("/fan/high", []() { handleCommand("fan-high"); });

  webServer.on("/sync/light/on", []() {
    isLightOn = true;
    pubSubClient.publish(mqttLightStateTopic, "ON");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.on("/sync/light/off", []() {
    isLightOn = false;
    pubSubClient.publish(mqttLightStateTopic, "OFF");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.on("/calibrate", []() {
    calibration();
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.begin();
  Serial.println("Web server started");
}

void setupMDNS() {
  if (MDNS.begin("artika-fan")) {
    Serial.println("mDNS responder started: artika-fan.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Error starting mDNS");
  }
}

#endif

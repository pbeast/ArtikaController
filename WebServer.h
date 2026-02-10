#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <map>
#include <string>
#include "RfCode.h"
#include "LogBuffer.h"

extern ESP8266WebServer webServer;
extern PubSubClient pubSubClient;
extern LogBuffer Log;

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
extern void sendRF(const char* commandName);

void handleWebRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link href='https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;600;700&family=Bebas+Neue&display=swap' rel='stylesheet'>";
  html += "<style>";
  html += "@keyframes blink{0%,50%{opacity:1}51%,100%{opacity:0}}";
  html += "@keyframes slideUp{from{opacity:0;transform:translateY(20px)}to{opacity:1;transform:translateY(0)}}";
  html += "@keyframes pulse{0%,100%{transform:scale(1)}50%{transform:scale(1.05)}}";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: 'IBM Plex Mono', monospace; background: #0a0a0a; color: #fff; min-height: 100vh; padding: 20px; ";
  html += "background-image: repeating-linear-gradient(0deg, transparent, transparent 2px, rgba(255,255,255,.03) 2px, rgba(255,255,255,.03) 4px); }";
  html += ".container { max-width: 600px; margin: 0 auto; animation: slideUp 0.6s ease-out; }";
  html += ".header { text-align: center; margin-bottom: 40px; border: 3px solid #fff; padding: 20px; background: #000; position: relative; }";
  html += ".header:before { content: ''; position: absolute; top: -6px; left: -6px; right: -6px; bottom: -6px; border: 1px solid #333; pointer-events: none; }";
  html += "h1 { font-family: 'Bebas Neue', sans-serif; font-size: 48px; letter-spacing: 8px; color: #fff; margin-bottom: 8px; text-transform: uppercase; }";
  html += ".subtitle { font-size: 11px; letter-spacing: 3px; color: #666; text-transform: uppercase; }";
  html += ".panel { background: #111; border: 3px solid #333; margin-bottom: 24px; position: relative; animation: slideUp 0.6s ease-out; animation-fill-mode: both; }";
  html += ".panel:nth-child(2) { animation-delay: 0.1s; }";
  html += ".panel:nth-child(3) { animation-delay: 0.2s; }";
  html += ".panel:nth-child(4) { animation-delay: 0.3s; }";
  html += ".panel:nth-child(5) { animation-delay: 0.4s; }";
  html += ".panel-header { background: #1a1a1a; padding: 12px 20px; border-bottom: 2px solid #333; display: flex; justify-content: space-between; align-items: center; }";
  html += ".panel-title { font-size: 13px; font-weight: 700; letter-spacing: 2px; text-transform: uppercase; color: #999; }";
  html += ".panel-body { padding: 24px; }";
  html += ".status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }";
  html += ".status-box { background: #000; border: 2px solid #222; padding: 20px; position: relative; }";
  html += ".status-box:before { content: ''; position: absolute; top: 8px; right: 8px; width: 8px; height: 8px; background: #333; }";
  html += ".status-label { font-size: 10px; color: #666; letter-spacing: 2px; margin-bottom: 12px; text-transform: uppercase; }";
  html += ".status-value { font-size: 32px; font-weight: 700; line-height: 1; }";
  html += ".status-value.on { color: #00ff41; text-shadow: 0 0 10px rgba(0,255,65,0.5); animation: pulse 2s infinite; }";
  html += ".status-value.off { color: #333; }";
  html += ".brightness-bar { display: flex; gap: 4px; margin-top: 8px; }";
  html += ".brightness-bar span { flex: 1; height: 8px; background: #222; border: 1px solid #333; }";
  html += ".brightness-bar span.active { background: #00ff41; box-shadow: 0 0 8px rgba(0,255,65,0.6); }";
  html += ".fan-visual { display: flex; gap: 8px; margin-top: 8px; justify-content: center; }";
  html += ".fan-blade { width: 12px; height: 24px; background: #222; border: 1px solid #333; }";
  html += ".fan-blade.active { background: #00b8ff; box-shadow: 0 0 8px rgba(0,184,255,0.6); }";
  html += ".controls { display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px; }";
  html += ".controls.triple { grid-template-columns: repeat(3, 1fr); }";
  html += ".controls.quad { grid-template-columns: repeat(4, 1fr); }";
  html += "button { background: #1a1a1a; color: #fff; border: 3px solid #333; padding: 18px 12px; cursor: pointer; font-family: 'IBM Plex Mono', monospace; ";
  html += "font-size: 13px; font-weight: 700; text-transform: uppercase; letter-spacing: 1px; transition: all 0.15s; position: relative; }";
  html += "button:before { content: ''; position: absolute; top: 4px; left: 4px; right: 4px; bottom: 4px; border: 1px solid #222; pointer-events: none; opacity: 0; transition: opacity 0.15s; }";
  html += "button:hover { background: #222; border-color: #00ff41; color: #00ff41; transform: translateY(-2px); }";
  html += "button:hover:before { opacity: 1; border-color: #00ff41; }";
  html += "button:active { transform: translateY(0); }";
  html += "button.primary { border-color: #00ff41; color: #00ff41; }";
  html += "button.primary:hover { background: #00ff41; color: #000; }";
  html += "button.danger { border-color: #ff0033; color: #ff0033; }";
  html += "button.danger:hover { background: #ff0033; color: #000; }";
  html += "button.warning { border-color: #ffaa00; color: #ffaa00; }";
  html += "button.warning:hover { background: #ffaa00; color: #000; }";
  html += ".sync-warning { background: #1a1400; border: 2px solid #ffaa00; padding: 16px; margin-top: 16px; }";
  html += ".sync-warning p { font-size: 11px; color: #ffaa00; margin-bottom: 12px; letter-spacing: 1px; }";
  html += ".sync-warning .controls { margin-top: 12px; }";
  html += ".footer { text-align: center; margin-top: 40px; padding: 20px; border-top: 1px solid #222; }";
  html += ".footer-line { font-size: 11px; color: #666; letter-spacing: 2px; margin: 4px 0; }";
  html += ".blink { animation: blink 1.5s infinite; }";
  html += "@media (max-width: 480px) { .controls, .controls.triple, .controls.quad { grid-template-columns: 1fr; } .status-grid { grid-template-columns: 1fr; } h1 { font-size: 36px; } }";

  // Monitor panel styles
  html += ".monitor-toggle { cursor: pointer; user-select: none; }";
  html += ".monitor-arrow { display: inline-block; transition: transform 0.2s; font-size: 12px; margin-right: 6px; }";
  html += ".monitor-arrow.open { transform: rotate(90deg); }";
  html += "#monitor-body { display: none; }";
  html += "#monitor-log { background: #000; border: 2px solid #222; height: 320px; overflow-y: auto; padding: 8px; font-family: 'IBM Plex Mono', monospace; font-size: 11px; }";
  html += "#monitor-log .log-line { color: #00ff41; line-height: 1.6; word-break: break-all; }";
  html += ".monitor-controls { display: flex; align-items: center; gap: 16px; margin-top: 12px; }";
  html += ".monitor-controls label { font-size: 11px; color: #666; letter-spacing: 1px; cursor: pointer; display: flex; align-items: center; gap: 6px; }";
  html += ".monitor-controls input[type=checkbox] { accent-color: #00ff41; }";

  html += "</style></head><body>";
  html += "<div class='container'>";

  html += "<div class='header'>";
  html += "<h1>ARTIKA</h1>";
  html += "<div class='subtitle'>IoT Control Interface</div>";
  html += "</div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'><div class='panel-title'>System Status</div><div style='font-size:10px;color:#666;'>LIVE <span class='blink'>●</span></div></div>";
  html += "<div class='panel-body'>";
  html += "<div class='status-grid'>";

  html += "<div class='status-box'><div class='status-label'>Light Status</div>";
  html += "<div class='status-value " + String(isLightOn ? "on" : "off") + "'>" + String(isLightOn ? "ON" : "OFF") + "</div></div>";

  html += "<div class='status-box'><div class='status-label'>Brightness</div>";
  html += "<div class='status-value'>" + String(brightnessLevel) + "</div>";
  html += "<div class='brightness-bar'>";
  for(int i = 1; i <= LED_BRIGHTNESS_LEVELS; i++) {
    html += "<span" + String(i <= brightnessLevel ? " class='active'" : "") + "></span>";
  }
  html += "</div></div>";

  html += "<div class='status-box'><div class='status-label'>Fan Status</div>";
  html += "<div class='status-value " + String(isFanOn ? "on" : "off") + "'>" + String(isFanOn ? "ON" : "OFF") + "</div></div>";

  html += "<div class='status-box'><div class='status-label'>Fan Speed</div>";
  html += "<div class='status-value'>";
  switch(currentFanSpeed) {
    case 0: html += "0"; break;
    case 1: html += "1"; break;
    case 2: html += "2"; break;
    case 3: html += "3"; break;
  }
  html += "</div>";
  html += "<div class='fan-visual'>";
  for(int i = 1; i <= 3; i++) {
    html += "<div class='fan-blade" + String(i <= currentFanSpeed ? " active" : "") + "'></div>";
  }
  html += "</div></div>";

  html += "</div></div></div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'><div class='panel-title'>Light Control</div></div>";
  html += "<div class='panel-body'>";
  html += "<div class='controls triple'>";
  html += "<button class='primary' onclick=\"location.href='/light/toggle'\">TOGGLE</button>";
  html += "<button onclick=\"location.href='/brightness/up'\">BRIGHT +</button>";
  html += "<button onclick=\"location.href='/brightness/down'\">BRIGHT –</button>";
  html += "</div></div></div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'><div class='panel-title'>Fan Control</div></div>";
  html += "<div class='panel-body'>";
  html += "<div class='controls quad'>";
  html += "<button class='danger' onclick=\"location.href='/fan/off'\">OFF</button>";
  html += "<button onclick=\"location.href='/fan/low'\">LOW</button>";
  html += "<button onclick=\"location.href='/fan/medium'\">MEDIUM</button>";
  html += "<button onclick=\"location.href='/fan/high'\">HIGH</button>";
  html += "</div></div></div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'><div class='panel-title'>State Synchronization</div></div>";
  html += "<div class='panel-body'>";
  html += "<div class='sync-warning'>";
  html += "<p>⚠ MANUAL OVERRIDE: USE IF STATE DESYNCHRONIZED</p>";
  html += "<p style='margin-top:8px;'><strong>Light State:</strong></p>";
  html += "<div class='controls'>";
  html += "<button class='primary' onclick=\"location.href='/sync/light/on'\">Light ON</button>";
  html += "<button class='warning' onclick=\"location.href='/sync/light/off'\">Light OFF</button>";
  html += "</div>";
  html += "<p style='margin-top:16px;'><strong>Fan State:</strong></p>";
  html += "<div class='controls quad'>";
  html += "<button class='warning' onclick=\"location.href='/sync/fan/off'\">Fan OFF</button>";
  html += "<button class='primary' onclick=\"location.href='/sync/fan/low'\">Fan LOW</button>";
  html += "<button class='primary' onclick=\"location.href='/sync/fan/medium'\">Fan MED</button>";
  html += "<button class='primary' onclick=\"location.href='/sync/fan/high'\">Fan HIGH</button>";
  html += "</div></div></div></div>";

  // System Monitor panel
  html += "<div class='panel' style='margin-top:24px;'>";
  html += "<div class='panel-header monitor-toggle' onclick='toggleMonitor()'>";
  html += "<div class='panel-title'><span class='monitor-arrow' id='mon-arrow'>&#9654;</span>SYSTEM MONITOR</div>";
  html += "<div style='font-size:10px;color:#666;' id='mon-status'></div>";
  html += "</div>";
  html += "<div id='monitor-body' class='panel-body' style='padding:16px;'>";
  html += "<div id='monitor-log'></div>";
  html += "<div class='monitor-controls'>";
  html += "<label><input type='checkbox' id='mon-follow' checked> FOLLOW</label>";
  html += "</div>";
  html += "</div></div>";

  // Monitor JavaScript
  html += "<script>";
  html += "var monOpen=false,sinceId=0,pollTimer=null;";
  html += "function toggleMonitor(){";
  html += "monOpen=!monOpen;";
  html += "document.getElementById('monitor-body').style.display=monOpen?'block':'none';";
  html += "document.getElementById('mon-arrow').classList.toggle('open',monOpen);";
  html += "if(monOpen){poll();pollTimer=setInterval(poll,2000);}";
  html += "else{if(pollTimer){clearInterval(pollTimer);pollTimer=null;}document.getElementById('mon-status').textContent='';}";
  html += "}";
  html += "function poll(){";
  html += "fetch('/api/logs?since='+sinceId).then(function(r){return r.json();}).then(function(d){";
  html += "sinceId=d.n;";
  html += "var el=document.getElementById('monitor-log');";
  html += "var lines=d.l;";
  html += "for(var i=0;i<lines.length;i++){";
  html += "var div=document.createElement('div');";
  html += "div.className='log-line';";
  html += "div.textContent=lines[i];";
  html += "el.appendChild(div);";
  html += "}";
  html += "while(el.children.length>250){el.removeChild(el.firstChild);}";
  html += "if(document.getElementById('mon-follow').checked&&lines.length>0){el.scrollTop=el.scrollHeight;}";
  html += "document.getElementById('mon-status').textContent='LIVE';";
  html += "}).catch(function(){document.getElementById('mon-status').textContent='ERR';});";
  html += "}";
  html += "</script>";

  html += "<div class='footer'>";
  html += "<div class='footer-line'>NETWORK: " + WiFi.localIP().toString() + "</div>";
  html += "<div class='footer-line'>HOSTNAME: artika-fan.local</div>";
  html += "</div>";

  html += "</div></body></html>";

  webServer.send(200, "text/html", html);
}

void handleCommand(String command) {
  static String lastCommand = "";
  static unsigned long lastCommandTime = 0;
  unsigned long currentTime = millis();

  // Debounce: ignore duplicate commands within 500ms
  if (command == lastCommand && (currentTime - lastCommandTime) < 500) {
    Log.println("Ignored duplicate web command (debounce)");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
    return;
  }

  lastCommand = command;
  lastCommandTime = currentTime;
  skipNextReceive = true;

  if (commandsToCodes.find(command.c_str()) != commandsToCodes.end()) {
    sendRF(command.c_str());

    // Update state
    if (command == "toggle-light") {
      isLightOn = !isLightOn;
      pubSubClient.publish(mqttLightStateTopic, isLightOn ? "ON" : "OFF");
    } else if (command == "brightness-up" && brightnessLevel < LED_BRIGHTNESS_LEVELS) {
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

void handleLogsAPI() {
  unsigned long since = 0;
  if (webServer.hasArg("since")) {
    since = strtoul(webServer.arg("since").c_str(), nullptr, 10);
  }
  String json = Log.getLogsSince(since);
  webServer.send(200, "application/json", json);
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

  webServer.on("/api/logs", handleLogsAPI);

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

  webServer.on("/sync/fan/off", []() {
    isFanOn = false;
    currentFanSpeed = 0;
    pubSubClient.publish(mqttFanStateTopic, "OFF");
    pubSubClient.publish(mqttFanSpeedStateTopic, "0");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.on("/sync/fan/low", []() {
    isFanOn = true;
    currentFanSpeed = 1;
    pubSubClient.publish(mqttFanStateTopic, "ON");
    pubSubClient.publish(mqttFanSpeedStateTopic, "1");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.on("/sync/fan/medium", []() {
    isFanOn = true;
    currentFanSpeed = 2;
    pubSubClient.publish(mqttFanStateTopic, "ON");
    pubSubClient.publish(mqttFanSpeedStateTopic, "2");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.on("/sync/fan/high", []() {
    isFanOn = true;
    currentFanSpeed = 3;
    pubSubClient.publish(mqttFanStateTopic, "ON");
    pubSubClient.publish(mqttFanSpeedStateTopic, "3");
    webServer.sendHeader("Location", "/");
    webServer.send(303);
  });

  webServer.begin();
  Log.println("Web server started");
}

void setupMDNS() {
  if (MDNS.begin("artika-fan")) {
    Log.println("mDNS responder started: artika-fan.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Log.println("Error starting mDNS");
  }
}

#endif

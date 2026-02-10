# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP8266-based RF to MQTT bridge for controlling Artika ceiling fans (tested with Artika Sunnyvale Fandelier) via Home Assistant. The device acts as a bidirectional bridge between 433MHz RF remote control signals and MQTT, with Home Assistant MQTT Discovery support and a built-in web interface.

## Hardware Configuration

- **ESP8266 Board**: NodeMCU, Wemos D1 Mini, or similar
- **Pin Assignments**:
  - GPIO 13: RF Receiver Data (`RC_RECEIVE_PIN`)
  - GPIO 12: RF Transmitter Data (`RC_SEND_PIN`)
  - GPIO 14: Physical Button (`BTN_PIN`) - optional
- **RF Protocol**: 433MHz, Protocol 1, Pulse Length 405µs

## Build and Upload

This is an Arduino sketch for ESP8266. Upload using Arduino IDE or PlatformIO:

**Arduino IDE:**
```bash
# Install required libraries via Library Manager:
# - RCSwitch
# - ESP8266WiFi (comes with ESP8266 board package)
# - ESP8266WebServer (comes with ESP8266 board package)
# - ESP8266mDNS (comes with ESP8266 board package)
# - PubSubClient

# Select board: Tools > Board > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)
# Upload via: Sketch > Upload
```

**Serial Monitor:**
```bash
# Baud rate: 115200
# View startup logs, WiFi connection, MQTT discovery, and RF signal debugging
```

## Configuration

All configuration is in `config.h` (gitignored). Create this file before uploading:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASSWORD = "YourPassword";

// MQTT Configuration
const char* MQTT_SERVER = "192.168.1.100";  // Can use hostname like "homesrv.local"
const int MQTT_SERVER_PORT = 1883;
const char* MQTT_USERNAME = "mqtt_username";
const char* MQTT_PASSWORD = "mqtt_password";

// Optional: Override default brightness levels (default: 5)
// #define LED_BRIGHTNESS_LEVELS 6

#endif
```

## Code Architecture

### File Organization

- **ArtikaController.ino**: Main sketch with setup(), loop(), MQTT callbacks, and state management
- **RfCode.h**: RF code definitions and command mapping
- **WebServer.h**: HTTP server handlers and HTML interface
- **config.h**: User configuration (gitignored)

### State Management

The system maintains synchronized state across three interfaces (RF remote, MQTT, web):

**State Variables** (in ArtikaController.ino):
- `isLightOn`, `brightnessLevel` (1 to `LED_BRIGHTNESS_LEVELS`, default 5)
- `isFanOn`, `currentFanSpeed` (0-3)

**Echo Prevention**:
- `skipNextReceive`: Set before sending RF commands to prevent processing our own transmissions
- `ignoreReceivedCodes`: Used to temporarily ignore all RF signals during multi-command sequences
- RF receiver enabled AFTER initial fan-off sync command to prevent echo processing

### Critical Timing and Debouncing

- **RF debouncing**: 100ms window (`processRFReceive`)
- **Button debouncing**: 50ms window (`handleButtonPress`)
- **Startup sync delay**: 500ms after sending fan-off before enabling receiver

### MQTT Integration

**Home Assistant MQTT Discovery**:
- Light entity: supports ON/OFF and brightness (1 to `LED_BRIGHTNESS_LEVELS` scale)
- Fan entity: percentage-based speed control (speed_range_min: 1, speed_range_max: 3)
- Buffer size increased to 768 bytes for discovery messages (`MQTT_MAX_PACKET_SIZE_FOR_DISCOVERY`)

**Topic Structure**:
- Command topics: `artika/{device}/set`
- State topics: `artika/{device}/state`
- Discovery published on connection in `publishHomeAssistantDiscovery()`

### RF Code Handling

**RfCode struct** (RfCode.h):
```cpp
struct RfCode {
  const char* sCodeWord;  // Binary representation (currently unused)
  unsigned long code;     // Decimal code value
  unsigned int length;    // Bit length (24 for all Artika commands)
};
```

**Command mapping**: `std::map<std::string, RfCode> commandsToCodes` allows lookup by command name or reverse lookup by code value.

**Adding new RF codes**:
1. Capture codes using Serial Monitor (uncomment `output()` call in `processRFReceive`)
2. Add to `fillCommandsMap()` in RfCode.h
3. Add corresponding state handling in `handleRFCommand()` and MQTT callback

### Web Interface State Synchronization

The web interface (`/sync/light/on`, `/sync/light/off`) updates internal state WITHOUT sending RF commands. This is critical for resolving desync when:
- Physical remote was used while controller was offline
- RF transmission was missed or interference occurred
- Light state toggled an odd number of times (toggle-based control)

### Light Control - Toggle vs Direct

**Important**: Light uses toggle command (`toggle-light`), not separate on/off commands. The MQTT callback compares requested state with current state and only sends toggle if they differ:

```cpp
if (requestedState != isLightOn) {
  // Send toggle command
  // Update state
  // Publish back to MQTT
}
```

### Loop Architecture

Main `loop()` handles:
1. MQTT connection maintenance (`reconnectMQTT()`)
2. MQTT message processing (`pubSubClient.loop()`)
3. Web server requests (`webServer.handleClient()`)
4. mDNS updates (`MDNS.update()`)
5. RF signal reception with echo prevention logic
6. Physical button monitoring

**State variables** are static within `loop()` for persistence (e.g., `lastReceivedCode`, `lastReceivedTime`).

## Device Access

- **Web Interface**: `http://artika-fan.local` or `http://<ESP_IP>`
- **Serial Monitor**: 115200 baud for debugging
- **MQTT**: Auto-discovered by Home Assistant when connected to same MQTT broker

## Modifying RF Codes

If codes need adjustment for different Artika models:
1. Comment out state handling in `handleRFCommand()`
2. Uncomment the `output()` debug line in `processRFReceive()`
3. Upload and use physical remote while monitoring Serial output
4. Record decimal codes and bit lengths
5. Update `fillCommandsMap()` in RfCode.h
6. Restore state handling

## Important Notes

- **MQTT buffer size**: Discovery messages are large; default PubSubClient buffer (256 bytes) is insufficient
- **Receiver timing**: Must enable receiver AFTER sending initial sync command to prevent echo
- **String-based MQTT server**: Uses `const char*` not `IPAddress` to support both IPs and hostnames
- **Global objects**: `mySwitchReceive`, `mySwitchSend`, `pubSubClient`, `webServer` are global for access across files

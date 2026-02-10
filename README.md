# Artika Fan Controller

ESP8266-based RF to MQTT bridge for controlling Artika ceiling fan with light via Home Assistant.

**Tested with:** Artika Sunnyvale Fandelier

**Compatibility:** May work with other Artika fan models using 433MHz RF remotes (RF codes may need adjustment)

## Features

- **RF Control**: Receives and transmits 433MHz RF signals to control Artika fan
- **MQTT Integration**: Full Home Assistant MQTT Discovery support
- **Web Interface**: Built-in web UI for manual control and state synchronization
- **Physical Button**: Optional button for light toggle control
- **State Synchronization**: Maintains state across RF remote, MQTT, and web interfaces

## Hardware Requirements

- ESP8266 board (NodeMCU, Wemos D1 Mini, etc.)
- 433MHz RF receiver module
- 433MHz RF transmitter module
- Optional: Push button for manual light control

## Pin Configuration

| Pin | Function |
|-----|----------|
| GPIO 13 | RF Receiver Data |
| GPIO 12 | RF Transmitter Data |
| GPIO 14 | Physical Button (optional) |

## Installation

1. Clone this repository
2. Create a `config.h` file with your WiFi and MQTT credentials (see Configuration section)
3. Install required Arduino libraries:
   - RCSwitch
   - ESP8266WiFi
   - ESP8266WebServer
   - ESP8266mDNS
   - PubSubClient
   - ArduinoOTA (comes with ESP8266 board package)
4. Upload to ESP8266 board (initial upload via USB required)

## Configuration

Create a `config.h` file with your WiFi and MQTT credentials:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASSWORD = "YourPassword";

// MQTT Configuration
const char* MQTT_SERVER = "192.168.1.100";
const int MQTT_SERVER_PORT = 1883;
const char* MQTT_USERNAME = "mqtt_username";
const char* MQTT_PASSWORD = "mqtt_password";

// Optional: Override default brightness levels (default: 5)
// #define LED_BRIGHTNESS_LEVELS 6

#endif
```

## Usage

**Important Initial State Assumptions:**
- The controller assumes the light is **ON** with **maximum brightness** at startup
- The controller assumes the fan is **OFF** at startup
- If your devices are in a different state when the controller starts, use the web interface State Sync section to correct the internal state
- See [Troubleshooting -> Light State Out of Sync](#light-state-out-of-sync) for details

### Web Interface

Access the web interface at:
- `http://artika-fan.local` (via mDNS)
- `http://<ESP8266_IP_ADDRESS>`

The web interface provides:
- Real-time device status display
- Light and fan control buttons
- Brightness adjustment
- State synchronization controls

### Home Assistant Integration

The device automatically registers with Home Assistant via MQTT Discovery. You'll see two entities:

1. **Artika Light** - Controllable light with brightness (configurable levels, default 5)
2. **Artika Fan** - Controllable fan with three speed levels (Low, Medium, High)

### MQTT Topics

**Command Topics:**
- `artika/light/set` - Light ON/OFF
- `artika/light/brightness/set` - Brightness (1 to LED_BRIGHTNESS_LEVELS, default 5)
- `artika/fan/set` - Fan ON/OFF
- `artika/fan/speed/set` - Fan speed (0-3)

**State Topics:**
- `artika/light/state` - Light state
- `artika/light/brightness/state` - Brightness level
- `artika/fan/state` - Fan state
- `artika/fan/speed/state` - Fan speed

### Over-The-Air (OTA) Updates

After the initial USB upload, you can update the firmware wirelessly over WiFi:

**Using Arduino IDE:**
1. Go to Tools > Port
2. Select "artika-fan at <IP_ADDRESS>" from the network ports
3. Upload as usual

**Using Arduino CLI:**
```bash
arduino-cli upload -p artika-fan.local --fqbn esp8266:esp8266:nodemcuv2 .
```

**Using PlatformIO:**
```bash
pio run --target upload --upload-port artika-fan.local
```

**Notes:**
- OTA is available at hostname `artika-fan.local` or the ESP8266's IP address
- Port 8266 is used for OTA updates
- RF receiver is automatically disabled during OTA to prevent interference
- First upload must be done via USB; subsequent uploads can use OTA

## Project Structure

```
ArtikaController/
├── ArtikaController.ino    # Main sketch
├── RfCode.h                 # RF code definitions
├── WebServer.h              # Web interface implementation
├── config.h                 # WiFi and MQTT credentials (gitignored)
├── .gitignore               # Git ignore rules
└── README.md                # This file
```

## RF Codes

The following RF codes are pre-configured for the Artika Sunnyvale Fandelier. If you have a different Artika fan model, you may need to capture and update these codes:

| Command | Code | Length |
|---------|------|--------|
| Toggle Light | 14432773 | 24 |
| Fan Off | 14432816 | 24 |
| Fan Low | 14432819 | 24 |
| Fan Medium | 14432794 | 24 |
| Fan High | 14432818 | 24 |
| Brightness Up | 14432793 | 24 |
| Brightness Down | 14432821 | 24 |

## Troubleshooting

### Light State Out of Sync

If the light state becomes out of sync with the physical device:
1. Navigate to the web interface
2. Under "State Sync" section, click the button that matches the actual physical state
3. This will update the controller's internal state without sending any RF commands

### MQTT Connection Issues

- Verify MQTT broker IP address and credentials
- Check that MQTT broker is running and accessible
- Review Serial Monitor output for connection errors

### RF Signal Issues

- Ensure RF modules are properly connected
- Check antenna connections
- Verify RF codes match your specific Artika fan model

### Fan Startup Behavior

**Normal behavior:** When starting the fan from OFF to LOW or MEDIUM speed, the fan may briefly spin at high speed before settling to the requested speed. This is a hardware characteristic of the Artika fan motor and is not a controller issue. The fan does this to overcome inertia and ensure reliable startup.

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

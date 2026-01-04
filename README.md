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
4. Upload to ESP8266 board

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

#endif
```

## Usage

**Important:** The controller assumes the light is **ON** with **maximum brightness (level 5)** when it starts. If your light is in a different state at startup, the controller's internal state will not match the physical device. See [Troubleshooting -> Light State Out of Sync](#light-state-out-of-sync) for how to resolve this.

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

1. **Artika Light** - Controllable light with brightness (1-5 levels)
2. **Artika Fan** - Controllable fan with three speed levels (Low, Medium, High)

### MQTT Topics

**Command Topics:**
- `artika/light/set` - Light ON/OFF
- `artika/light/brightness/set` - Brightness (1-5)
- `artika/fan/set` - Fan ON/OFF
- `artika/fan/speed/set` - Fan speed (0-3)

**State Topics:**
- `artika/light/state` - Light state
- `artika/light/brightness/state` - Brightness level
- `artika/fan/state` - Fan state
- `artika/fan/speed/state` - Fan speed

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

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

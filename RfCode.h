#ifndef RFCODE_H
#define RFCODE_H

struct RfCode {
  unsigned long code;
  unsigned int length;
};

// Default RF codes - override in config.h if your remote uses different codes
#ifndef RF_CODE_TOGGLE_LIGHT
#define RF_CODE_TOGGLE_LIGHT 14432773
#endif
#ifndef RF_CODE_FAN_OFF
#define RF_CODE_FAN_OFF 14432816
#endif
#ifndef RF_CODE_FAN_LOW
#define RF_CODE_FAN_LOW 14432819
#endif
#ifndef RF_CODE_FAN_MEDIUM
#define RF_CODE_FAN_MEDIUM 14432794
#endif
#ifndef RF_CODE_FAN_HIGH
#define RF_CODE_FAN_HIGH 14432818
#endif
#ifndef RF_CODE_BRIGHTNESS_UP
#define RF_CODE_BRIGHTNESS_UP 14432793
#endif
#ifndef RF_CODE_BRIGHTNESS_DOWN
#define RF_CODE_BRIGHTNESS_DOWN 14432821
#endif

std::map<std::string, RfCode> commandsToCodes;

void fillCommandsMap() {
  commandsToCodes["toggle-light"] =     RfCode{RF_CODE_TOGGLE_LIGHT, 24};

  commandsToCodes["fan-off"] =          RfCode{RF_CODE_FAN_OFF, 24};
  commandsToCodes["fan-low"] =          RfCode{RF_CODE_FAN_LOW, 24};
  commandsToCodes["fan-medium"] =       RfCode{RF_CODE_FAN_MEDIUM, 24};
  commandsToCodes["fan-high"] =         RfCode{RF_CODE_FAN_HIGH, 24};

  commandsToCodes["brightness-up"] =    RfCode{RF_CODE_BRIGHTNESS_UP, 24};
  commandsToCodes["brightness-down"] =  RfCode{RF_CODE_BRIGHTNESS_DOWN, 24};
}

#endif

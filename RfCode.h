#ifndef RFCODE_H
#define RFCODE_H

struct RfCode {
  const char* sCodeWord;
  unsigned long code;
  unsigned int length;
};

std::map<std::string, RfCode> commandsToCodes;

void fillCommandsMap() {
  commandsToCodes["toggle-light"] =     RfCode{"110111000011101000000101", 14432773, 24};

  commandsToCodes["fan-off"] =          RfCode{"110111000011101000110000", 14432816, 24};
  commandsToCodes["fan-low"] =          RfCode{"110111000011101000110011", 14432819, 24};
  commandsToCodes["fan-medium"] =       RfCode{"110111000011101000011010", 14432794, 24};
  commandsToCodes["fan-high"] =         RfCode{"110111000011101000110010", 14432818, 24};

  commandsToCodes["brightness-up"] =    RfCode{"110111000011101000011001", 14432793, 24};
  commandsToCodes["brightness-down"] =  RfCode{"110111000011101000110101", 14432821, 24};
}

#endif

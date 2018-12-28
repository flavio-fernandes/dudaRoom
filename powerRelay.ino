#include "common.h"

#define POWER_RELAY1   8
#define POWER_RELAY2   9

void initPowerRelay()
{
  pinMode(POWER_RELAY1, OUTPUT);
  pinMode(POWER_RELAY2, OUTPUT);

  setPowerRelay();
}

void setPowerRelay()
{
  const bool isOn = getStateFlag(stateFlagPowerRelay);
  setLedState(LED_POWER_RELAY, isOn ? ledStateOn : ledStateGoingOff);
  digitalWrite(POWER_RELAY1, isOn ? HIGH : LOW);
  digitalWrite(POWER_RELAY2, isOn ? LOW : HIGH);
}

void flipPowerRelay()
{
  flipStateFlag(stateFlagPowerRelay);
  setPowerRelay();
}

#ifndef _COMMON_H

#define _COMMON_H

#define DEBUG 1

#define SPKNUM 10

#define LED_POWER_RELAY   0
#define LED_LEARN_TRIGGER 1
#define LED_LEARN_CMD     2

typedef enum {
  loopInterruptNone,
  loopInterruptButton1,
  loopInterruptButton2,
  loopInterruptButton3,
} LoopInterrupt;

typedef enum {
  ledStateOff = 0,
  ledStateOn,
  ledStateGoingOff,
  ledStateGoingOn,
  ledStateHeartUp,    // going on right now...
  ledStateHeartDown,  // going off right now...
} LedState;

typedef enum {
  troubleIdNoEasyVr = 1,
} TroubleId;

// FWD decls... nvRam
void initFromEeprom();
void checkForEepromUpdate();

// FWD decls... lights
void initLights();
void setLedState(int ledId, int /*LedState*/ ledState);
void flipLedState(int ledId);
void updateLedDisplay();
int getLedState(int ledId);
void doLearnDance();
void troubleLightHalt(int /*TroubleId*/ value);

// FWD decls... buttons
void initButtons();
int /*LoopInterrupt*/ checkButtons();
void buttonPenaltyBoxTick(const unsigned long tickValue);
bool checkClearButtonPress(int /*LoopInterrupt*/ loopInterruptButton);

// FWD decls... powerRelay
void initPowerRelay();
void flipPowerRelay();
void setPowerRelay();


// FWD decls... dudaRoom
typedef enum {
  stateFlagPowerRelay = 0,
  stateFlagLast = 6  // check eeprom limits
} StateFlag;

bool getStateFlag(int /*StateFlag*/ flag);
void setStateFlag(int /*StateFlag*/ flag);
void clearStateFlag(int /*StateFlag*/ flag);
void flipStateFlag(int /*StateFlag*/ flag);

typedef struct {

  int nextTriggerIndex;
  int nextCommandIndex;
  int flags; // based on StateFlag

  unsigned long next100time;  // 100 milliseconds timer
  unsigned long next500time;  // 500 milliseconds timer
  unsigned long next1000time; // 1 second timer
} State;

extern State state;

#endif // _COMMON_H


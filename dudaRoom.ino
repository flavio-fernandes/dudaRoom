#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #include "SoftwareSerial.h"
  SoftwareSerial port(12,13);
#else // Arduino 0022 - use modified NewSoftSerial
  #include "WProgram.h"
  #include "NewSoftSerial.h"
  NewSoftSerial port(12,13);
#endif

#include "common.h"
#include "pitches.h"


#include "EasyVR.h"
EasyVR easyvr(port);
EasyVRBridge bridge;

State state;

static const unsigned long noteDuration = 1000/6;
static const int8_t triggerGroup = 2;
static const int8_t cmdGroup = 1;

bool getStateFlag(int /*StateFlag*/ flag) { return bitRead(state.flags, flag) == 1; }
void setStateFlag(int /*StateFlag*/ flag) { bitSet(state.flags, flag); }
void clearStateFlag(int /*StateFlag*/ flag) { bitClear(state.flags, flag); }
void flipStateFlag(int /*StateFlag*/ flag) {
  if (getStateFlag(flag)) clearStateFlag(flag); else setStateFlag(flag);
}

// ----------------------------------------

void initGlobals() {
  memset(&state, 0, sizeof(state));

  easyvr.setPinOutput(EasyVR::IO1, LOW);
  easyvr.setPinOutput(EasyVR::IO2, LOW);
  easyvr.setPinOutput(EasyVR::IO3, LOW);
  easyvr.setTimeout(7); // seconds
  easyvr.setLanguage(EasyVR::ENGLISH);
  easyvr.setLevel(EasyVR::HARD);  // EASY vs NORMAL, HARD...
  //easyvr.setKnob(EasyVR::LOOSE);

  const unsigned long now = millis();
  state.next100time = now + 100;
  state.next500time = now + 500;
  state.next1000time = now + 1000;
}

void clearAllTriggers();
void clearAllCommands();

void checkClearButton()
{
  if (checkClearButtonPress(loopInterruptButton1)) clearAllTriggers();
  if (checkClearButtonPress(loopInterruptButton2)) clearAllCommands();
  if (checkClearButtonPress(loopInterruptButton3)) {
    setLedState(LED_LEARN_TRIGGER /*ledId*/, ledStateOn);
    setLedState(LED_LEARN_CMD /*ledId*/, ledStateOn);
    setLedState(LED_POWER_RELAY /*ledId*/, ledStateOn);
    updateLedDisplay();
    
    easyvr.resetAll();
    memset(&state, 0, sizeof(state));
    setPowerRelay();

    setLedState(LED_LEARN_TRIGGER /*ledId*/, ledStateOff);
    setLedState(LED_LEARN_CMD /*ledId*/, ledStateOff);
    setLedState(LED_POWER_RELAY /*ledId*/, ledStateOff);
  }
}

void setup() {
  // bridge mode?
  if (bridge.check()) { cli(); bridge.loop(0, 1, 12, 13); }

  pinMode(SPKNUM, OUTPUT);

#ifdef DEBUG
  Serial.begin(9600);
#endif
  port.begin(9600);

  // stage 1
  initGlobals();
  initLights();
  initButtons();

  if (!easyvr.detect()) { 
#ifdef DEBUG
    Serial.println("EasyVR not detected!");
#endif
    troubleLightHalt(troubleIdNoEasyVr);
  }

  // stage 2
  initFromEeprom();
  initPowerRelay();

  // stage 3
  checkClearButton();

  doLearnDance();
}

// ----------------------------------------

int loopInterrupt;

bool loopCheck() {
  const unsigned long now = millis();

  // check it it is time to do something interesting. Note that now will wrap and we will
  // get a 'blib' every 50 days. How cares, right?
  //
  if (state.next100time <= now) {
    updateLedDisplay();

    state.next100time = now + 100;
  } else if (state.next500time <= now) {

    state.next500time = now + 500;
  } else if (state.next1000time <= now) {
    buttonPenaltyBoxTick(1000 /*tickValue*/);    
    checkForEepromUpdate();

    state.next1000time = now + 1000;
  } else {
    // nothing to do... let's rest a little
    delay(10);
  }

  loopInterrupt = checkButtons();
  return (loopInterrupt == loopInterruptNone);
}

void command_loop();  // fwd
void learnModeTrigger();  // fwd
void learnModeCommand();  // fwd

void loop() {
  easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
#ifdef DEBUG
  Serial.println("Listening for trigger...");
#endif
  easyvr.recognizeCommand(triggerGroup /*EasyVR::TRIGGER*/);
  loopInterrupt = (int) loopInterruptNone;
  do {
    if (!loopCheck()) { easyvr.stop(); break; }
  } while ( !easyvr.hasFinished() );
  easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off

  switch (loopInterrupt) {
  case loopInterruptButton1:
    setLedState(LED_LEARN_TRIGGER /*ledId*/, ledStateOn);
    learnModeTrigger();
    setLedState(LED_LEARN_TRIGGER /*ledId*/, ledStateOff);
    break;
  case loopInterruptButton2:
    setLedState(LED_LEARN_CMD /*ledId*/, ledStateOn);
    learnModeCommand();
    setLedState(LED_LEARN_CMD /*ledId*/, ledStateOff);
    break;
  case loopInterruptButton3:
    flipPowerRelay();
    break;
  case loopInterruptNone:
  default:
    break;
  }

  if (loopInterrupt != loopInterruptNone || easyvr.getCommand() < 0) return;

  // if we made it here, we were not interrupted and the trigger was recognized
  setLedState(LED_LEARN_CMD /*ledId*/, ledStateOn);
  setLedState(LED_LEARN_TRIGGER /*ledId*/, ledStateHeartUp);
  updateLedDisplay();  // flush state, so leds are 'in sync'
  setLedState(LED_LEARN_CMD /*ledId*/, ledStateHeartDown);

  tone(SPKNUM, NOTE_A4, noteDuration);
  command_loop();
  setLedState(LED_LEARN_CMD /*ledId*/, ledStateOff);
  setLedState(LED_LEARN_TRIGGER /*ledId*/, ledStateGoingOff);
}

// ----------------------------------------

void command_loop()
{
  easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
#ifdef DEBUG
  Serial.println("Listening for command...");
#endif
  easyvr.recognizeCommand(cmdGroup);
  loopInterrupt = (int) loopInterruptNone;
  do {
    if (!loopCheck()) { easyvr.stop(); break; }
  } while ( !easyvr.hasFinished() );
  easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off

  const int8_t cmdIndex = easyvr.getCommand();
  if (loopInterrupt != loopInterruptNone || cmdIndex < 0) {
#ifdef DEBUG
    Serial.println("No command match");
#endif
    tone(SPKNUM, NOTE_E2, noteDuration);
    return;
  }

  // if we made it here, we were not interrupted and the command was recognized
  if (cmdIndex % 2 == 0) {
    setStateFlag(stateFlagPowerRelay);
  } else {
    clearStateFlag(stateFlagPowerRelay);
  }
  setPowerRelay();
  tone(SPKNUM, NOTE_A7, noteDuration);

#ifdef DEBUG
  Serial.print("Command accepted. Flags: "); Serial.println(state.flags);
#endif
}

// ----------------------------------------

void learnModeCommand()
{ 
  // hijack power relay led to show whether we are learning on or off
  const int origLedState = getLedState( (int) LED_POWER_RELAY );
  const bool isOn = (state.nextCommandIndex % 2) == 0;
  setLedState(LED_POWER_RELAY, isOn ? ledStateOn : ledStateOff);

  easyVrLearnMode(cmdGroup, &state.nextCommandIndex);

  // restore power relay led value
  setLedState(LED_POWER_RELAY, origLedState);
}

void learnModeTrigger() { easyVrLearnMode(triggerGroup, &state.nextTriggerIndex); }

bool easyVrLearnMode(const int8_t group, int* indexPtr)
{
  const int8_t index = (int8_t) *indexPtr;

  updateLedDisplay();  // because train does not like disruption...

  // remove previous trainning
  easyvr.eraseCommand(group, index);
  easyvr.removeCommand(group, index);

  easyvr.addCommand(group, index);    // add custom back

#ifdef DEBUG
  Serial.print("Learning on group: "); Serial.print((int)group);
  Serial.print(" index: "); Serial.println((int)index);
#endif

  easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)
  easyvr.trainCommand(group, index);
  loopInterrupt = (int) loopInterruptNone;
  do {
    // train does not like disruption: do not interfere until it is done
    // XXXX  if (!loopCheck()) { easyvr.stop(); break; }  XXX
  } while (!easyvr.hasFinished());
  easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED off (not listening)

  // if (loopInterrupt != loopInterruptNone) return false;

  int16_t err = easyvr.getError();
  if (err >= 0) {
#ifdef DEBUG
    Serial.print("Learn error: "); Serial.println(err, HEX);
#endif
    tone(SPKNUM, NOTE_E2, noteDuration);
    return false;
  }

  if (++(*indexPtr) > 30) *indexPtr = 0; 

  doLearnDance();
  return true;
}

void clearEasyVr(const int8_t group, int* indexPtr)
{
  for(int8_t index = 0; index <= 30; ++index) {
    easyvr.eraseCommand(group, index);
    easyvr.removeCommand(group, index);
  }
  *indexPtr = 0; 
#ifdef DEBUG
  Serial.print("Cleared all entries on on group: "); Serial.println((int)group);
#endif
}
void clearAllTriggers() { clearEasyVr(triggerGroup, &state.nextTriggerIndex); }
void clearAllCommands() { clearEasyVr(cmdGroup, &state.nextCommandIndex); }

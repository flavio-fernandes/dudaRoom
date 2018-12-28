#include "common.h"

const int buttonPin[] = { A3, A4, A5 };
const int buttonCount = sizeof(buttonPin) / sizeof(int);

struct ButtonInfo {
  int highValue;
  int lowValue;
  unsigned long penaltyBoxCounter;  // millis until buton can be used again
};

struct ButtonInfo buttonInfo[buttonCount];

void buttonPenaltyBoxTick(const unsigned long tickValue) {
  int i;
  for (i = 0; i < buttonCount; ++i) {
    if (buttonInfo[i].penaltyBoxCounter > tickValue) buttonInfo[i].penaltyBoxCounter -= tickValue;
    else buttonInfo[i].penaltyBoxCounter = 0;
  }
}

int getButtonMidValue(const int buttonIndex) {
  return (buttonInfo[buttonIndex].highValue + buttonInfo[buttonIndex].lowValue) / 2;
}

void initButtons()
{
   for (int i = 0; i < buttonCount; ++i) {
     pinMode(buttonPin[i], INPUT);
     buttonInfo[i].highValue = 400;
     buttonInfo[i].lowValue = 300;
     buttonInfo[i].penaltyBoxCounter = 0;
   }

}

bool checkForButtonPress(const int buttonIndex, bool allowLongPress)
{
  const int noteDuration = 1000/7;
  const unsigned long penaltyBoxInterval = 2000;  // millisecs in the penalty box
  
  int currAnalogRead;
  const unsigned long buttonDown = millis();
  const unsigned long buttonDownCheckInterval = 100; // how often to read value and determine if button is still down
  unsigned long buttonPressedCountDown = 230; // millisenconds button must stay pressed down until we declare it is pressed

  // do analog read and declare button pressed when its analog value jumps
  // by a big factor. We expect value to be big and go low when button is
  // pressed, because we are usinf normally closed buttons
  //
  while (buttonPressedCountDown > 0) {
    currAnalogRead = analogRead(buttonPin[buttonIndex]);

    // update highest and lowest reads for this button
    //
    if (buttonInfo[buttonIndex].lowValue > currAnalogRead) buttonInfo[buttonIndex].lowValue = currAnalogRead;
    if (buttonInfo[buttonIndex].highValue < currAnalogRead) buttonInfo[buttonIndex].highValue = currAnalogRead;

    // if button is not pressed, we are done. Because Buttons 0-2 are NC and button 3 id NO, we need to know
    // which button we are talking about in order to do this right.
    //

#ifdef BUTTON_IS_NC
      if (currAnalogRead > getButtonMidValue(buttonIndex)) return false;
#else
      if (currAnalogRead < getButtonMidValue(buttonIndex)) return false;
#endif

    // if this button is in the penalty box, skip any further checking on it
    //
    if (buttonInfo[buttonIndex].penaltyBoxCounter > 0) {
      tone(SPKNUM, NOTE_E2, noteDuration);

      // this could be the case of a stuck button. Do not let him out of the penalty box
      //
      if (buttonInfo[buttonIndex].penaltyBoxCounter > 150) {
	buttonInfo[buttonIndex].penaltyBoxCounter = penaltyBoxInterval;
      }
      
      return false;
    }
    
    delay(buttonDownCheckInterval);
    if (buttonPressedCountDown > buttonDownCheckInterval) buttonPressedCountDown -= buttonDownCheckInterval;
    else break;
  }

  // put button on penalty box: it shall not be considered down for that anount of time
  //
  buttonInfo[buttonIndex].penaltyBoxCounter = penaltyBoxInterval;

#ifdef DEBUG
  Serial.print("Button was pressed down: "); Serial.print(buttonIndex);
  Serial.print(" lowValue: "); Serial.print(buttonInfo[buttonIndex].lowValue);
  Serial.print(" highValue: "); Serial.print(buttonInfo[buttonIndex].highValue);
  Serial.print(" currAnalogRead: "); Serial.print(currAnalogRead);
  Serial.println("");
#endif

  noTone(SPKNUM);
  tone(SPKNUM, NOTE_C3, noteDuration);

  // button debounce
  //
  buttonPressedCountDown = 4999; // max amount of time we will tolerate button down
  while (1) {
    delay(1);
    currAnalogRead = analogRead(buttonPin[buttonIndex]);
      
    // Because Buttons 0-2 are NC and button 3 id NO, we need to know
    // which button we are talking about in order to do this right.
    //      
#ifdef BUTTON_IS_NC
      if (currAnalogRead > getButtonMidValue(buttonIndex)) break;
#else
      if (currAnalogRead < getButtonMidValue(buttonIndex)) break;
#endif

    if (buttonPressedCountDown > 1) --buttonPressedCountDown; else {
      // button down for too long. We will simply ignore it was ever pressed and
      // let the penalty box logix take it from here
      //
      return false;
    }
  }
  const unsigned long buttonUp = millis();

#ifdef DEBUG
  Serial.print("Button was released: "); Serial.print(buttonIndex);
  Serial.print(" after "); Serial.print(buttonUp - buttonDown);
  Serial.println(" milliseconds");
#endif

  // was it down for longer then 2 seconds? If so, and millis() did not wrap on us, take note on that
  const boolean longButtonPress = buttonUp > buttonDown && buttonUp - buttonDown > 1800;

  tone(SPKNUM, longButtonPress ? NOTE_D6 : NOTE_C4, noteDuration);

  // if button was down for too long, ignore it...
  return (!longButtonPress || allowLongPress);
}

int /*LoopInterrupt*/ checkButtons()
{
  if (checkForButtonPress(0, false)) return (int) loopInterruptButton1;
  if (checkForButtonPress(1, false)) return (int) loopInterruptButton2;
  if (checkForButtonPress(2, false)) return (int) loopInterruptButton3;

  return (int) loopInterruptNone;
}

bool checkClearButtonPress(int /*LoopInterrupt*/ loopInterruptButton)
{
  int buttonIndex;
  switch (loopInterruptButton) {
  case loopInterruptButton1: buttonIndex = 0; break;
  case loopInterruptButton2: buttonIndex = 1; break;
  case loopInterruptButton3: buttonIndex = 2; break;
  default:
    return false;
    break;
  }
  return checkForButtonPress(buttonIndex, true /*allowLongPress*/);
}

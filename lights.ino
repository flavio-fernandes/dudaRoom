#include "common.h"

const int ledPin[] = { A0, A1, A2 };
const int ledCount = sizeof(ledPin) / sizeof(int);

const int maxLedIntensity = 255;
const int ledIntensityIncrement = 15;

struct LedInfo {
  LedState ledState;   
  unsigned int ledIntensity; // 0 => off maxLedIntensity => on
};

LedInfo ledInfo[ledCount];

void initLights()
{
   for (int i = 0; i < ledCount; ++i) {
     pinMode(ledPin[i], OUTPUT);
     digitalWrite(ledPin[i], LOW);
   }
}

int getLedState(int ledId)
{
  if (ledId >= ledCount) return (int) ledStateOff;  // sanity
  return (int) ledInfo[ledId].ledState;
}

void setLedState(int ledId, int /*LedState*/ ledState)
{
  if (ledId >= ledCount) return;  // sanity
  ledInfo[ledId].ledState = (LedState) ledState;
}

void flipLedState(int ledId)
{
  if (ledId >= ledCount) return;  // sanity

  if (ledInfo[ledId].ledState == ledStateOff) {
    ledInfo[ledId].ledState = ledStateGoingOn;
  } else {
    ledInfo[ledId].ledState = ledStateGoingOff;
  }
}

void updateLedDisplayCommon(bool forceWrite)
{
  for (int i=0; i < ledCount; ++i) {
    switch (ledInfo[i].ledState) {
    case ledStateOff:
      if (ledInfo[i].ledIntensity == 0 && !forceWrite) continue;  // skip analogWrite...
      ledInfo[i].ledIntensity = 0;
      break;
    case ledStateOn:
      if (ledInfo[i].ledIntensity == maxLedIntensity && !forceWrite) continue;  // skip analogWrite...
      ledInfo[i].ledIntensity = maxLedIntensity;
      break;
    case ledStateGoingOff:
    case ledStateHeartDown:
      if (ledInfo[i].ledIntensity <= ledIntensityIncrement) {
	ledInfo[i].ledIntensity = 0;
	ledInfo[i].ledState = (ledInfo[i].ledState == ledStateGoingOff) ? ledStateOff : ledStateHeartUp;
      } else {
	ledInfo[i].ledIntensity -= ledIntensityIncrement;
      }
      break;
    case ledStateGoingOn:
    case ledStateHeartUp:
      if (ledInfo[i].ledIntensity + ledIntensityIncrement >= maxLedIntensity) {
	ledInfo[i].ledIntensity = maxLedIntensity;
	ledInfo[i].ledState = (ledInfo[i].ledState == ledStateGoingOn) ? ledStateOn : ledStateHeartDown;
      } else {
	ledInfo[i].ledIntensity += ledIntensityIncrement;
      }
      break;
    }
    analogWrite(ledPin[i], ledInfo[i].ledIntensity);
  }
}

void updateLedDisplay()
{
  updateLedDisplayCommon(false /*forceWrite*/);
}

void doLearnDance()
{
  const int danceMoves = ledCount * 5;
  int i;
  LedInfo orgLedInfo[ledCount];

  tone(SPKNUM, NOTE_A7, noteDuration);
  tone(SPKNUM, NOTE_C4, noteDuration);

  // save away lights state
  for (i = 0; i < ledCount; ++i) orgLedInfo[i] = ledInfo[i];

  for (int danceMove = 0; danceMove < danceMoves; ++danceMove) {
    const int ledOn = danceMove % ledCount;
    for (i = 0; i < ledCount; ++i) {
      digitalWrite(ledPin[i], i == ledOn ? HIGH : LOW);
    }
    delay(50);
  }

  // restore lights
  for (i = 0; i < ledCount; ++i) ledInfo[i] = orgLedInfo[i];
  updateLedDisplayCommon(true /*forceWrite*/);
}

void troubleLightHalt(int /*TroubleId*/ troubleId)
{
  for (int i = 0; i < ledCount; ++i) digitalWrite(ledPin[i], bitRead(troubleId, i) ? HIGH : LOW);
  while (1) { delay(3000); tone(SPKNUM, NOTE_C4, noteDuration); };
}

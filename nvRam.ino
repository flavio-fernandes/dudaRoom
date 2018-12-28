#include "common.h"
#include <EEPROM.h>

#define EEPROM_MAGIC_SEED 3
#define EEPROM_MAGIC_SEED_INCR 1

#define EEPROM_MAGIC_BITS 2
#define EEPROM_MAGIC_MASK  ((1 << EEPROM_MAGIC_BITS) - 1)
#define EEPROM_MAGIC_SHIFT (8 - EEPROM_MAGIC_BITS)
#define EEPROM_MAGIC_VALUE_MASK1 0xc0
#define EEPROM_MAGIC_VALUE_MASK2 0x3f


// There are up to 512 slots, but it takes a while to read them all.
// So we will do fewer since they should not burn out too fast.
#define EEPROM_SLOTS_PER_SECTION     13


struct EepromSection {
       const char* name;
       int* currValuePtr;
       int base;
       int lastEepromValue;
       int nextEepromAddr;
       int nextEepromMagic;
};

typedef enum {
	sectionTypeTrigger = 0,
	sectionTypeCommand,
	sectionTypeFlags,
	sectionTypeLast
} SectionType;

struct EepromSection eepromSections[sectionTypeLast];


void initFromEepromCommon(struct EepromSection* section)
{
  int i = 0;
  byte currByte = EEPROM.read(0 + section->base);
  int currMagic = (int) ((currByte & EEPROM_MAGIC_VALUE_MASK1) >> EEPROM_MAGIC_SHIFT);
  int currEepromValue = (int) (currByte & EEPROM_MAGIC_VALUE_MASK2);

  // initial values, only needed if slot 0 is not useable
  //
  section->lastEepromValue = 0;
  section->nextEepromAddr = 0;
  section->nextEepromMagic = EEPROM_MAGIC_SEED & EEPROM_MAGIC_MASK;

  while (1) {
    ++i;
    if (i > EEPROM_SLOTS_PER_SECTION) break;

    // if magic got out of sync, it means that the writes wrapped and the value
    // we just read is the oldest in the circular buffer
    //
    if (i != 1 && currMagic != section->nextEepromMagic) break;

    section->lastEepromValue = currEepromValue;
    section->nextEepromAddr = (i == EEPROM_SLOTS_PER_SECTION) ? 0 : i; 
    section->nextEepromMagic = ((currMagic + EEPROM_MAGIC_SEED_INCR) & EEPROM_MAGIC_MASK);

#ifdef DEBUG2
  Serial.print("eeprom name: "); Serial.print(section->name);
  Serial.print(" READ nextEepromAddr: "); Serial.print(section->nextEepromAddr, DEC);
  Serial.print(" nextEepromMagic: "); Serial.print(section->nextEepromMagic, DEC);
  Serial.print(" lastEepromValue: 0x"); Serial.print(section->lastEepromValue, HEX);
  Serial.println("");
#endif

    if (i == EEPROM_SLOTS_PER_SECTION) break; // done reading last eeprom slot

    // prepare for next read address, to see if it is newer than the previous read
    //
    currByte = EEPROM.read(i + section->base);
    currMagic = (int) ((currByte & EEPROM_MAGIC_VALUE_MASK1) >> EEPROM_MAGIC_SHIFT);
    currEepromValue = (int) (currByte & EEPROM_MAGIC_VALUE_MASK2);
  }

#ifdef DEBUG
  Serial.print("INIT Eeprom called for name: "); Serial.print(section->name);
  Serial.print(" base: "); Serial.print(section->base, DEC);
  Serial.print(" nextEepromAddr: "); Serial.print(section->nextEepromAddr, DEC);
  Serial.print(" nextEepromMagic: "); Serial.print(section->nextEepromMagic, DEC);
  Serial.print(" lastEepromValue: 0x"); Serial.print(section->lastEepromValue, HEX);
  Serial.println("");
#endif

  // Last but not least: place value to its proper place
  *section->currValuePtr = section->lastEepromValue;
}

void initFromEeprom()
{
     memset(&eepromSections[0], 0, sizeof(eepromSections[0]) * sectionTypeLast);

     eepromSections[sectionTypeTrigger].base = EEPROM_SLOTS_PER_SECTION * sectionTypeTrigger;
     eepromSections[sectionTypeTrigger].name = "trigger";
     eepromSections[sectionTypeTrigger].currValuePtr = &state.nextTriggerIndex;

     eepromSections[sectionTypeCommand].base = EEPROM_SLOTS_PER_SECTION * sectionTypeCommand;
     eepromSections[sectionTypeCommand].name = "command";
     eepromSections[sectionTypeCommand].currValuePtr = &state.nextCommandIndex;

     eepromSections[sectionTypeFlags].base = EEPROM_SLOTS_PER_SECTION * sectionTypeFlags;
     eepromSections[sectionTypeFlags].name = "flags";
     eepromSections[sectionTypeFlags].currValuePtr = &state.flags;

     for (int i=0; i < sectionTypeLast; ++i) initFromEepromCommon(&eepromSections[i]);
}

// ======

void checkForEepromUpdateCommon(struct EepromSection* section)
{
  const int newEepromValue = *section->currValuePtr & EEPROM_MAGIC_VALUE_MASK2;

  if (section->lastEepromValue == newEepromValue) return; // eeprom is up-to-date (on the bits it cares for...)

  byte currByte = (byte) (section->nextEepromMagic << EEPROM_MAGIC_SHIFT) & EEPROM_MAGIC_VALUE_MASK1;
  currByte |= (byte) newEepromValue;

  EEPROM.write(section->nextEepromAddr + section->base, currByte);

  section->lastEepromValue = newEepromValue;
  section->nextEepromAddr += 1; if (section->nextEepromAddr >= EEPROM_SLOTS_PER_SECTION) section->nextEepromAddr = 0;
  section->nextEepromMagic = ((section->nextEepromMagic + EEPROM_MAGIC_SEED_INCR) & EEPROM_MAGIC_MASK);

#ifdef DEBUG
  Serial.print("UPDATE Eeprom wrote new entry. name: "); Serial.print(section->name);
  Serial.print(" nextEepromAddr: "); Serial.print(section->nextEepromAddr, DEC);
  Serial.print(" nextEepromMagic: "); Serial.print(section->nextEepromMagic, DEC);
  Serial.print(" lastEepromValue: "); Serial.print(section->lastEepromValue, HEX);
  Serial.println("");
#endif
}

void checkForEepromUpdate()
{
     for (int i=0; i < sectionTypeLast; ++i) checkForEepromUpdateCommon(&eepromSections[i]);
}

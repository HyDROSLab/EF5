#include "TimeUnit.h"
#include "Defines.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

const char *TimeUnitText[] = {
    "y", "m", "d", "h", "u", "s",
};

const unsigned long TimeUnitSeconds[] = {
    60 * 60 * 24 * 365, 60 * 60 * 24 * 30, 60 * 60 * 24, 60 * 60, 60, 1,
};

unsigned long TimeUnit::GetTimeInSec() { return timeInSec; }

int TimeUnit::GetTimeModifier() { return timeModifier; }

SUPPORTED_TIME_UNITS TimeUnit::GetTimeUnit() { return timeUnit; }

SUPPORTED_TIME_UNITS TimeUnit::ParseUnit(char *unitText) {
  size_t len = strlen(unitText);
  SUPPORTED_TIME_UNITS result = TIME_UNIT_QTY;
  char modifier[CONFIG_MAX_LEN];
  char *unitStr = unitText;
  int intModifier = 0;
  unsigned long time = 0;

  for (size_t i = 0; i < len; i++) {
    if ((*unitStr >= '0' && *unitStr <= '9')) {
      modifier[i] = *unitStr;
      unitStr++;
    } else if (i == 0) {
      modifier[0] = '1';
      modifier[1] = 0;
      break;
    } else {
      modifier[i] = 0;
      break;
    }
  }

  intModifier = atoi(modifier);

  for (int i = 0; i < TIME_UNIT_QTY; i++) {
    if (!strcasecmp(TimeUnitText[i], unitStr)) {
      result = (SUPPORTED_TIME_UNITS)i;
      break;
    }
  }

  if (result != TIME_UNIT_QTY) {
    time = TimeUnitSeconds[result];
    time = (unsigned long)(intModifier * time);
    timeInSec = time;
  }

  timeUnit = result;
  timeModifier = intModifier;

  return result;
}

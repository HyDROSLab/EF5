#ifndef TIMEUNIT_H
#define TIMEUNIT_H

enum SUPPORTED_TIME_UNITS {
  YEARS,
  MONTHS,
  DAYS,
  HOURS,
  MINUTES,
  SECONDS,
  TIME_UNIT_QTY,
};

extern const char *TimeUnitText[];
extern const unsigned long TimeUnitSeconds[];

class TimeUnit {

public:
  unsigned long GetTimeInSec();
  int GetTimeModifier();
  SUPPORTED_TIME_UNITS GetTimeUnit();
  SUPPORTED_TIME_UNITS ParseUnit(char *unitText);

private:
  unsigned long timeInSec;
  int timeModifier;
  SUPPORTED_TIME_UNITS timeUnit;
};

#endif

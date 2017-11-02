#ifndef TIME_VAR_H
#define TIME_VAR_H

#include "Defines.h"
#include "TimeUnit.h"
#include <time.h>

class TimeVar {

public:
  bool LoadTime(char *time);
  bool LoadTimeExcel(char *time);
  void Increment(TimeUnit *inc);
  void Decrement(TimeUnit *inc);
  tm *GetTM();

  TimeVar &operator=(const TimeVar &rhs);
  friend bool operator==(const TimeVar &lhs, const TimeVar &rhs);
  friend bool operator<(const TimeVar &lhs, const TimeVar &rhs);
  friend bool operator<=(const TimeVar &lhs, const TimeVar &rhs);

  time_t currentTimeSec;

private:
  tm currentTime;

  time_t port_timegm(struct tm *tm);
};

#endif

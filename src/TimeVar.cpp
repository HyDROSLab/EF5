#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <time.h>
#endif
#include "Messages.h"
#include "TimeVar.h"
#include <stdlib.h>

#ifdef _WIN32
int is_leap(int year);
int days_from_0(int year);
int days_from_1970(int year);
int days_from_1jan(int year, int month, int day);
time_t internal_timegm(struct tm *t);
#endif

bool TimeVar::LoadTime(char *time) {

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;

  memset(&currentTime, 0, sizeof(tm));
  currentTime.tm_mday = 1;
  currentTime.tm_isdst = -1;

  int num = sscanf(time, "%04d%02d%02d%02d%02d%02d", &year, &month, &day, &hour,
                   &minute, &second);

  switch (num) {
  case 6:
    currentTime.tm_sec = second;
  case 5:
    currentTime.tm_min = minute;
  case 4:
    currentTime.tm_hour = hour;
  case 3:
    currentTime.tm_mday = day;
  case 2:
    currentTime.tm_mon = month - 1;
  case 1:
    currentTime.tm_year = year - 1900;
    break;
  default:
    ERROR_LOGF("Unable to process time \"%s\"!", time);
    return false;
  }

  // This cleans up currentTime
  currentTimeSec = port_timegm(&currentTime); // For portability use mktime
                                              // (while setting TZ environmental
                                              // variable) instead of timegm

  return true;
}

bool TimeVar::LoadTimeExcel(char *time) {

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  size_t len = strlen(time);
  memset(&currentTime, 0, sizeof(tm));
  currentTime.tm_mday = 1;
  currentTime.tm_isdst = -1;

  bool yearMonthDay = false;
  if (time[2] != '-' && time[2] != '/') {
    yearMonthDay = true;
  }

  int num = 1;
  char *end;
  if (!yearMonthDay) {
    if (len > 2) {
      time[2] = ' '; // Month-Day
    }
    if (len > 5) {
      time[5] = ' '; // Day-Year
    }
    if (len > 13) {
      time[13] = ' '; // Hour:Minute
    }
    if (len > 16) {
      time[16] = ' ';
    }
    month = (int)strtol(time, &end, 10);
    if (*end != '\0') {
      num++;
      day = (int)strtol(end, &end, 10);
      if (*end != '\0') {
        num++;
        year = (int)strtol(end, &end, 10);
        if (*end != '\0') {
          num++;
          hour = (int)strtol(end, &end, 10);
          if (*end != '\0') {
            num++;
            minute = (int)strtol(end, &end, 10);
            if (*end != '\0') {
              num++;
              second = (int)strtol(end, &end, 10);
            }
          }
        }
      }
    }
  } else {
    if (len > 4) {
      time[4] = ' '; // Year-Month
    }
    if (len > 7) {
      time[7] = ' '; // Month-Day
    }
    if (len > 13) {
      time[13] = ' '; // Hour:Minute
    }
    if (len > 16) {
      time[16] = ' ';
    }
    year = (int)strtol(time, &end, 10);
    if (*end != '\0') {
      num++;
      month = (int)strtol(end, &end, 10);
      if (*end != '\0') {
        num++;
        day = (int)strtol(end, &end, 10);
        if (*end != '\0') {
          num++;
          hour = (int)strtol(end, &end, 10);
          if (*end != '\0') {
            num++;
            minute = (int)strtol(end, &end, 10);
            if (*end != '\0') {
              num++;
              second = (int)strtol(end, &end, 10);
            }
          }
        }
      }
    }
  }

  switch (num) {
  case 6:
    currentTime.tm_sec = second;
  case 5:
    currentTime.tm_min = minute;
  case 4:
    currentTime.tm_hour = hour;
  case 3:
    currentTime.tm_mday = day;
  case 2:
    currentTime.tm_mon = month - 1;
  case 1:
    if (year < 1900) {
      if (year < 50) {
        year += 2000;
      } else {
        year += 1900;
      }
    }
    currentTime.tm_year = year - 1900;
    break;
  default:
    ERROR_LOGF("Unable to process time \"%s\"!", time);
    return false;
  }

  // This cleans up currentTime
  currentTimeSec = port_timegm(&currentTime); // For portability use mktime
                                              // (while setting TZ environmental
                                              // variable) instead of timegm

  /*INFO_LOGF(
      "Input \"%s\", year %i, month %i, day %i, hour %i, minutes %i, epoch %li",
      time, currentTime.tm_year + 1900, currentTime.tm_mon + 1,
      currentTime.tm_mday, currentTime.tm_min, currentTime.tm_sec,
      currentTimeSec);*/

  return true;
}

void TimeVar::Decrement(TimeUnit *inc) {

  switch (inc->GetTimeUnit()) {
  case YEARS:
    currentTime.tm_year -= inc->GetTimeModifier();
    break;
  case MONTHS:
    currentTime.tm_mon -= inc->GetTimeModifier();
    break;
  case DAYS:
    currentTime.tm_mday -= inc->GetTimeModifier();
    break;
  case HOURS:
    currentTime.tm_hour -= inc->GetTimeModifier();
    break;
  case MINUTES:
    currentTime.tm_min -= inc->GetTimeModifier();
    break;
  case SECONDS:
    currentTime.tm_sec -= inc->GetTimeModifier();
    break;
  default:
    break; // Should be __builtin_unreachable();
  }

  currentTime.tm_isdst = -1;

  // Clean up currentTime
  currentTimeSec = port_timegm(&currentTime); // For portability use mktime
                                              // (while setting TZ environmental
                                              // variable) instead of timegm
}

void TimeVar::Increment(TimeUnit *inc) {

  switch (inc->GetTimeUnit()) {
  case YEARS:
    currentTime.tm_year += inc->GetTimeModifier();
    break;
  case MONTHS:
    currentTime.tm_mon += inc->GetTimeModifier();
    break;
  case DAYS:
    currentTime.tm_mday += inc->GetTimeModifier();
    break;
  case HOURS:
    currentTime.tm_hour += inc->GetTimeModifier();
    break;
  case MINUTES:
    currentTime.tm_min += inc->GetTimeModifier();
    break;
  case SECONDS:
    currentTime.tm_sec += inc->GetTimeModifier();
    break;
  default:
    break; // Should be __builtin_unreachable();
  }

  currentTime.tm_isdst = -1;

  // Clean up currentTime
  currentTimeSec = port_timegm(&currentTime); // For portability use mktime
                                              // (while setting TZ environmental
                                              // variable) instead of timegm
}

tm *TimeVar::GetTM() { return &currentTime; }

TimeVar &TimeVar::operator=(const TimeVar &rhs) {

  memcpy(&currentTime, &rhs.currentTime, sizeof(tm));
  currentTimeSec = port_timegm(&currentTime);
  return *this;
}

bool operator==(const TimeVar &lhs, const TimeVar &rhs) {

  return lhs.currentTimeSec == rhs.currentTimeSec;

  /*if (lhs.currentTime.tm_min != rhs.currentTime.tm_min) {
    return false;
  }

  if (lhs.currentTime.tm_hour != rhs.currentTime.tm_hour) {
    return false;
  }

  if (lhs.currentTime.tm_mday != rhs.currentTime.tm_mday) {
    return false;
  }

  if (lhs.currentTime.tm_year != rhs.currentTime.tm_year) {
    return false;
  }

  if (lhs.currentTime.tm_mon != rhs.currentTime.tm_mon) {
    return false;
  }

  if (lhs.currentTime.tm_sec != rhs.currentTime.tm_sec) {
    return false;
  }

  return true;*/
}

bool operator<(const TimeVar &lhs, const TimeVar &rhs) {

  return lhs.currentTimeSec < rhs.currentTimeSec;

  /*
   int diff = lhs.currentTime.tm_year - rhs.currentTime.tm_year;
   if (diff > 0) {
     return false;
   } else if (diff < 0) {
     return true;
   }

   diff = lhs.currentTime.tm_mon - rhs.currentTime.tm_mon;
   if (diff > 0) {
     return false;
   } else if (diff < 0) {
     return true;
   }

   diff = lhs.currentTime.tm_mday - rhs.currentTime.tm_mday;
   if (diff > 0) {
     return false;
   } else if (diff < 0) {
     return true;
   }

   diff = lhs.currentTime.tm_hour - rhs.currentTime.tm_hour;
   if (diff > 0) {
     return false;
   } else if (diff < 0) {
     return true;
   }

   diff = lhs.currentTime.tm_min - rhs.currentTime.tm_min;
   if (diff > 0) {
     return false;
   } else if (diff < 0) {
     return true;
   }

   diff = lhs.currentTime.tm_sec - rhs.currentTime.tm_sec;
   if (diff > 0) {
     return false;
   } else if (diff < 0) {
     return true;
   }

   return false;*/
}

bool operator<=(const TimeVar &lhs, const TimeVar &rhs) {

  return lhs.currentTimeSec <= rhs.currentTimeSec;

  /*int diff = lhs.currentTime.tm_year - rhs.currentTime.tm_year;
  if (diff > 0) {
    return false;
  } else if (diff < 0) {
    return true;
  }

  diff = lhs.currentTime.tm_mon - rhs.currentTime.tm_mon;
  if (diff > 0) {
    return false;
  } else if (diff < 0) {
    return true;
  }

  diff = lhs.currentTime.tm_mday - rhs.currentTime.tm_mday;
  if (diff > 0) {
    return false;
  } else if (diff < 0) {
    return true;
  }

  diff = lhs.currentTime.tm_hour - rhs.currentTime.tm_hour;
  if (diff > 0) {
    return false;
  } else if (diff < 0) {
    return true;
  }

  diff = lhs.currentTime.tm_min - rhs.currentTime.tm_min;
  if (diff > 0) {
    return false;
  } else if (diff < 0) {
    return true;
  }

  diff = lhs.currentTime.tm_sec - rhs.currentTime.tm_sec;
  if (diff > 0) {
    return false;
  } else if (diff < 0) {
    return true;
  }

  return true;*/
}

time_t TimeVar::port_timegm(struct tm *tm) {

#ifdef _WIN32
  time_t result = mktime(tm);
  struct tm *newtm = gmtime(&result);
  // if (!newtm) {
  //	ERROR_LOGF("Got null from gmtime! %i, %i %i %i %i %i %i", result,
  //tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  // } else {
  tm->tm_year = newtm->tm_year;
  tm->tm_mon = newtm->tm_mon;
  tm->tm_mday = newtm->tm_mday;
  tm->tm_hour = newtm->tm_hour;
  tm->tm_min = newtm->tm_min;
  tm->tm_sec = newtm->tm_sec;
  //	}
  return result; // internal_timegm(tm);
#else
  return timegm(tm);
#endif
}

#ifdef WIN32
inline int is_leap(int year) {
  if (year % 400 == 0)
    return 1;
  if (year % 100 == 0)
    return 0;
  if (year % 4 == 0)
    return 1;
  return 0;
}
inline int days_from_0(int year) {
  year--;
  return 365 * year + (year / 400) - (year / 100) + (year / 4);
}
inline int days_from_1970(int year) {
  static const int days_from_0_to_1970 = days_from_0(1970);
  return days_from_0(year) - days_from_0_to_1970;
}
inline int days_from_1jan(int year, int month, int day) {
  static const int days[2][12] = {
      {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
      {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};
  return days[is_leap(year)][month - 1] + day - 1;
}

inline time_t internal_timegm(struct tm *t) {
  int year = t->tm_year + 1900;
  int month = t->tm_mon;
  if (month > 11) {
    year += month / 12;
    month %= 12;
  } else if (month < 0) {
    int years_diff = (-month + 11) / 12;
    year -= years_diff;
    month += 12 * years_diff;
  }
  month++;
  int day = t->tm_mday;
  int day_of_year = days_from_1jan(year, month, day);
  int days_since_epoch = days_from_1970(year) + day_of_year;

  time_t seconds_in_day = 3600 * 24;
  time_t result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour +
                  60 * t->tm_min + t->tm_sec;

  return result;
}
#endif

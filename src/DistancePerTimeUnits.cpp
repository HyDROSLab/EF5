#include "DistancePerTimeUnits.h"
#include "Defines.h"
#include <cstring>

bool DistancePerTimeUnits::ParseUnit(char *units) {
  char distance[CONFIG_MAX_LEN];
  char *timeStr = units;
  size_t len = strlen(units);

  // This processes something in the format of distance/time
  // It splits up the distance portion from the time portion allowing each to be
  // individually parsed
  for (size_t i = 0; i < len; i++) {
    if (*timeStr != '/') {
      distance[i] = *timeStr;
      timeStr++;
    } else {
      distance[i] = 0;
      timeStr++; // Move past the /
      break;
    }
  }

  SUPPORTED_DISTANCE_UNITS distResult = dist.ParseUnit(distance);
  SUPPORTED_TIME_UNITS timeResult = time.ParseUnit(timeStr);

  if (distResult == DIST_UNIT_QTY || timeResult == TIME_UNIT_QTY) {
    return false;
  } else {
    return true;
  }
}

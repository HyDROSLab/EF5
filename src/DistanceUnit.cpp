#include "DistanceUnit.h"
#include <cstring>

const char *DistanceUnitText[] = {
    "m",
    "mm",
    "cm",
};

SUPPORTED_DISTANCE_UNITS DistanceUnit::ParseUnit(char *unitText) {
  for (int i = 0; i < DIST_UNIT_QTY; i++) {
    if (!strcasecmp(DistanceUnitText[i], unitText)) {
      return (SUPPORTED_DISTANCE_UNITS)i;
    }
  }

  return DIST_UNIT_QTY;
}

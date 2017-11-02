#ifndef DISTANCEUNIT_H
#define DISTANCEUNIT_H

enum SUPPORTED_DISTANCE_UNITS {
  METERS,
  MILLIMETERS,
  CENTIMETERS,
  DIST_UNIT_QTY,
};

extern const char *DistanceUnitText[];

class DistanceUnit {

public:
  SUPPORTED_DISTANCE_UNITS ParseUnit(char *unitText);
};

#endif

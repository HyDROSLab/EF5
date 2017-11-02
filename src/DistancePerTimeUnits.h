#ifndef DISTANCEPERTIMEUNITS_H
#define DISTANCEPERTIMEUNITS_H

#include "DistanceUnit.h"
#include "TimeUnit.h"

class DistancePerTimeUnits {

public:
  bool ParseUnit(char *units);
  TimeUnit *GetTime() { return &time; }
  DistanceUnit *GetDist() { return &dist; }

private:
  DistanceUnit dist;
  TimeUnit time;
};

#endif

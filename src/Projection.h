#ifndef PROJECTION_H
#define PROJECTION_H

#include "Defines.h"

enum PROJECTIONS {
  PROJECTION_GEOGRAPHIC,
  PROJECTION_LAEA,
};

class Projection {

public:
  virtual float GetLen(float x, float y, FLOW_DIR dir) = 0;
  virtual float GetArea(float x, float y) = 0;
  virtual void SetCellSize(float newCellSize) = 0;
  virtual void ReprojectPoint(float lon, float lat, float *x, float *y) = 0;
  virtual void UnprojectPoint(float x, float y, float *lon, float *lat) = 0;
};

#endif

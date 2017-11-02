#ifndef LAEA_PROJECTION_H
#define LAEA_PROJECTION_H

#include "Projection.h"

class LAEAProjection : public Projection {

public:
  LAEAProjection();
  ~LAEAProjection();
  float GetLen(float x, float y, FLOW_DIR dir);
  float GetArea(float x, float y);
  void ReprojectPoint(float lon, float lat, float *x, float *y);
  void UnprojectPoint(float x, float y, float *lon, float *lat);
  void SetCellSize(float newCellSize);

private:
  float cellSize;
  float area;
  float metersSNPerCell;
  float metersDiagPerCell;
  float stan_par, cent_lon, a;
};

#endif

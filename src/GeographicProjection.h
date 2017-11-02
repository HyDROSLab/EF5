#ifndef GEOGRAPHIC_PROJECTION_H
#define GEOGRAPHIC_PROJECTION_H

#include "Projection.h"

class GeographicProjection : public Projection {

public:
  GeographicProjection();
  ~GeographicProjection();
  float GetLen(float x, float y, FLOW_DIR dir);
  float GetArea(float x, float y);
  void ReprojectPoint(float lon, float lat, float *x, float *y);
  void UnprojectPoint(float x, float y, float *lon, float *lat);
  void SetCellSize(float newCellSize);

private:
  float cellSize;
  double metersSNPerCell;
};

#endif

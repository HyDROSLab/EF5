#include "GeographicProjection.h"
#include "Messages.h"
#include <cmath>
#include <cstdio>

GeographicProjection::GeographicProjection() {}

GeographicProjection::~GeographicProjection() {}

float GeographicProjection::GetLen(float x, float y, FLOW_DIR dir) {

  switch (dir) {
  case FLOW_NORTH:
  case FLOW_SOUTH:
    return metersSNPerCell;
  case FLOW_EAST:
  case FLOW_WEST:
    return metersSNPerCell * cos(TORADIANS(y));
  case FLOW_NORTHEAST:
  case FLOW_SOUTHEAST:
  case FLOW_SOUTHWEST:
  case FLOW_NORTHWEST:
    return sqrt(pow(metersSNPerCell * cos(TORADIANS(y)), 2.0) +
                pow(metersSNPerCell, 2.0));
  default:
    return 0; // We would do __builtin_unreachable(); but Apple's GCC lags
              // behind
  }

  return 0;
}

float GeographicProjection::GetArea(float x, float y) {

  float lenEW = metersSNPerCell * cos(TORADIANS(y));
  return (lenEW * metersSNPerCell) / 1000000; // We want km^2
}

void GeographicProjection::ReprojectPoint(float lon, float lat, float *x,
                                          float *y) {
  // Reproject a lat lon point into this projection
  // Well we're already in geographic here...

  *x = lon;
  *y = lat;
}

void GeographicProjection::UnprojectPoint(float x, float y, float *lon,
                                          float *lat) {
  // Project a lat lon to a lat lon
  // Oh wait... already done!

  *lon = x;
  *lat = y;
}

void GeographicProjection::SetCellSize(float newCellSize) {
  cellSize = newCellSize;
  metersSNPerCell = cellSize * 110574; // 110574m per degree of latitude!
}

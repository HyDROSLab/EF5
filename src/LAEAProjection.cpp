#include "LAEAProjection.h"
#include "Defines.h"
#include "Messages.h"
#include <cmath>
#include <cstdio>

LAEAProjection::LAEAProjection() {
  stan_par = TORADIANS(45.0);
  cent_lon = TORADIANS(-100.0);
  a = 6370997.0;
}

LAEAProjection::~LAEAProjection() {}

float LAEAProjection::GetLen(float x, float y, FLOW_DIR dir) {

  switch (dir) {
  case FLOW_NORTH:
  case FLOW_SOUTH:
  case FLOW_EAST:
  case FLOW_WEST:
    return metersSNPerCell;
  case FLOW_NORTHEAST:
  case FLOW_SOUTHEAST:
  case FLOW_SOUTHWEST:
  case FLOW_NORTHWEST:
    return metersDiagPerCell;
  default:
    return 0; // We would do __builtin_unreachable(); but Apple's GCC lags
              // behind
  }

  return 0;
}

float LAEAProjection::GetArea(float x, float y) { return area; }

void LAEAProjection::ReprojectPoint(float lon, float lat, float *x, float *y) {
  // Reproject a geographic point into a LAEA point...

  lon = TORADIANS(lon);
  lat = TORADIANS(lat);

  // Equations from mathworld.wolfram.com
  float kPrime = sqrt(2.0 / (1.0 + sin(stan_par) * sin(lat) +
                             cos(stan_par) * cos(lat) * cos(lon - cent_lon)));
  float projX = kPrime * cos(lat) * sin(lon - cent_lon);
  float projY = kPrime * (cos(stan_par) * sin(lat) -
                          sin(stan_par) * cos(lat) * cos(lon - cent_lon));

  *x = a * projX;
  *y = a * projY;
}

void LAEAProjection::UnprojectPoint(float x, float y, float *lon, float *lat) {
  // Project a LAEA point into geographic!
  // Equations from mathworld.wolfram.com

  float projX = x / a;
  float projY = y / a;

  float rho = sqrt(pow(projX, 2) + pow(projY, 2));
  float c = 2 * asin(0.5 * rho);
  float geoLat =
      asin(cos(c) * sin(stan_par) + (projY * sin(c) * cos(stan_par)) / rho);
  float geoLon =
      cent_lon + atan((projX * sin(c)) / (rho * cos(stan_par) * cos(c) -
                                          projY * sin(stan_par) * sin(c)));

  *lon = TODEGREES(geoLon);
  *lat = TODEGREES(geoLat);
}

void LAEAProjection::SetCellSize(float newCellSize) {
  cellSize = newCellSize;
  metersSNPerCell = cellSize;
  metersDiagPerCell = sqrt(pow(cellSize, 2) + pow(cellSize, 2));
  area = pow(cellSize, 2) / 1000000; // Want km^2
}

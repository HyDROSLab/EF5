#ifndef GRID_H
#define GRID_H

#include "BoundingBox.h"
#include <cstdio>
#include <math.h>

struct GridLoc {
  long x;
  long y;
};

struct RefLoc {
  float x;
  float y;
};

class Grid {

public:
  long numCols;
  long numRows;
  BoundingBox extent;
  double cellSize;
  unsigned short modelType, geographicType, geodeticDatum;
  bool geoSet;

  bool IsSpatialMatch(const Grid *testGrid) {
    bool nxnyMatch =
        ((numCols == testGrid->numCols) && (numRows == testGrid->numRows));
    return nxnyMatch;
    /*if (!nxnyMatch) {
            return false;
    }
    bool cellSize = (fabsf(cellSize - testGrid->cellSize) < 0.001);
    return cellSize;*/
    //    return ((numCols == testGrid->numCols) && (numRows ==
    //    testGrid->numRows)
    //            && (extent.left == testGrid->extent.left) && (extent.bottom ==
    //            testGrid->extent.bottom)
    //           && (cellSize == testGrid->cellSize));
  }

  bool GetGridLoc(float lon, float lat, GridLoc *pt) {
    float xDiff = lon - extent.left;
    float yDiff = extent.top - lat;
    float xLoc = xDiff / cellSize;
    float yLoc = yDiff / cellSize;
    pt->x = (long)xLoc;
    pt->y = (long)yLoc;

    if (pt->x < 0) {
      pt->x = 0;
    } else if (pt->x >= numCols) {
      pt->x = numCols - 1;
    }

    if (pt->y < 0) {
      pt->y = 0;
    } else if (pt->y >= numRows) {
      pt->y = numRows - 1;
    }

    if (extent.left > lon || extent.right < lon || extent.bottom > lat ||
        extent.top < lat) {
      return false; // This point isn't in the grid!
    } else {
      return true;
    }
  }

  bool GetRefLoc(long x, long y, RefLoc *pt) {
    pt->x = (float)x * cellSize + extent.left;
    pt->y = extent.top - (float)y * cellSize;
    return true;
  }
};

class FloatGrid : public Grid {

public:
  FloatGrid() {
    data = NULL;
    backingStore = NULL;
    geoSet = false;
  }
  ~FloatGrid() {
    if (data) {
      if (!backingStore) {
        for (long i = 0; i < numRows; i++) {
          if (data[i]) {
            delete[] data[i];
          }
        }
      } else {
        delete[] backingStore;
      }
      delete[] data;
    }
  }
  float noData;
  float **data;
  float *backingStore;
};

class LongGrid : public Grid {

public:
  LongGrid() {
    data = NULL;
    geoSet = false;
  }
  ~LongGrid() {
    if (data) {
      for (long i = 0; i < numRows; i++) {
        if (data[i]) {
          delete[] data[i];
        }
      }
      delete[] data;
    }
  }
  long noData;
  long **data;
};

#endif

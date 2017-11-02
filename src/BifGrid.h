#ifndef BIF_GRID_H
#define BIF_GRID_H

#include "Grid.h"

#pragma pack(push)
#pragma pack(1)
struct BifHeader {
  int ncols;
  int nrows;
  float xllcor;
  float yllcor;
  float cellsize;
  float nodata;
  char empty[26];
};
#pragma pack(pop)

FloatGrid *ReadFloatBifGrid(char *file);

#endif

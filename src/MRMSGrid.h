#ifndef MRMS_GRID_H
#define MRMS_GRID_H

#include "Grid.h"

#pragma pack(push)
#pragma pack(1)
struct MRMSHeader2D {
  int yr;
  int mo;
  int day;
  int hr;
  int min;
  int sec;
  int nx;
  int ny;
  int nz;
  char projection[4];
  int map_scale;
  int trulat1;
  int trulat2;
  int trulon;
  int nw_lon;
  int nw_lat;
  int xy_scale;
  int dx;
  int dy;
  int dxy_scale;
  int grid_height;
  int z_scale;
  int placeholders[10];
  char temp_varname[20];
  char temp_varunit[6];
  int var_scale;
  int missing_val;
  int nradars;
  char radar_id[4];
};
#pragma pack(pop)

FloatGrid *ReadFloatMRMSGrid(char *file, FloatGrid *grid);
FloatGrid *ReadFloatMRMSGrid(char *file);

#endif

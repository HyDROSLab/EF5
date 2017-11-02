#include "MRMSGrid.h"
#include "Messages.h"
#include <cstdio>
#include <math.h>
#include <zlib.h>

FloatGrid *ReadFloatMRMSGrid(char *file, FloatGrid *grid) {

  gzFile fileH;

  fileH = gzopen(file, "rb");
  if (fileH == NULL) {
    return NULL;
  }

  short int *binary_data = 0;
  float nw_lon, nw_lat;
  float dx; //, dy;

  MRMSHeader2D header;
  if (gzread(fileH, &header, sizeof(MRMSHeader2D)) != sizeof(MRMSHeader2D)) {
    WARNING_LOGF("MRMS file %s corrupt? (missing header)", file);
    gzclose(fileH);
    return NULL;
  }

  if (gzseek(fileH, (header.nradars - 1) * 4, SEEK_CUR) == -1) {
    WARNING_LOGF("MRMS file %s corrupt? (missing header radars)", file);
    gzclose(fileH);
    return NULL;
  }

  /*-------------------------*/
  /*** 3. Read binary data ***/
  /*-------------------------*/

  int num = header.nx * header.ny;
  binary_data = new short int[num];

  if (!binary_data) {
    WARNING_LOGF("MRMS file %s too large (out of memory) with %i points", file,
                 num);
    gzclose(fileH);
    return NULL;
  }

  // read data array
  if (gzread(fileH, binary_data, num * sizeof(short int)) !=
      num * (int)(sizeof(short int))) {
    WARNING_LOGF("MRMS file %s corrupt?", file);
    delete[] binary_data;
    gzclose(fileH);
    return NULL;
  }

  gzclose(fileH);

  float *__restrict__ backingStore = new float[num];
  const float scalef = (float)header.var_scale;
  int li = 0;
  /*#pragma omp parallel for
   for (li = 0; li < num; li += 4) {
   backingStore[li] = ((float)binary_data[li]) / scalef;
   backingStore[li+1] = ((float)binary_data[li+1]) / scalef;
   backingStore[li+2] = ((float)binary_data[li+2]) / scalef;
   backingStore[li+3] = ((float)binary_data[li+3]) / scalef;
   }
   */

  bool badFile = false;

  //#pragma omp parallel for
  for (li = 0; li < num; li++) {
    backingStore[li] = ((float)binary_data[li]) / scalef;
  }

  /*#pragma omp parallel for
   for (li = 0; li < num; li++) {
   if (backingStore[li] > 500.0) {
   badFile = true;
   }
   }*/

  dx = header.dx / float(header.dxy_scale);
  // dy = header.dy/float(header.dxy_scale);
  nw_lon = (float)header.nw_lon / (float)header.map_scale - (dx / 2.0);
  nw_lat = (float)header.nw_lat / (float)header.map_scale - (dx / 2.0);
  if (!grid) {
    grid = new FloatGrid();
    grid->numCols = header.nx;
    grid->numRows = header.ny;
    grid->cellSize = dx;
    grid->extent.top = nw_lat;
    grid->extent.left = nw_lon;
    grid->backingStore = backingStore;
    grid->data = new float *[grid->numRows]();
    if (!grid->data) {
      WARNING_LOGF("MRMS file %s too large (out of memory) with %li rows", file,
                   grid->numRows);
      delete[] binary_data;
      delete grid;
      return NULL;
    }
    grid->noData = -999.0;
    const int numRows = (int)grid->numRows;
    const int nX = header.nx;
    for (int i = 0; i < numRows; i++) {
      int realI = numRows - i - 1;
      int index = realI * nX;
      grid->data[i] = &(backingStore[index]);
    }
  }

  /*#pragma omp parallel for
   for (long i = 0; i < grid->numRows; i++) {
   #pragma omp parallel for
   for (int j = 0; j < grid->numCols; j++) {
   int realI = grid->numRows - i - 1;
   int index = realI*nx + j;
   float floatData = 0;
   floatData = ((float)binary_data[index]) / (float)var_scale;
   grid->data[i][j] = floatData;
   if (floatData > 500.0) {
   badFile = true;
   }
   }
   }*/

  delete[] binary_data;

  if (badFile) {
    printf("Rejecting %s for values > 500.0 ", file);
    delete grid;
    return NULL;
  }

  // Fill in the rest of the BoundingBox
  grid->extent.bottom = grid->extent.top - grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  return grid;
}

FloatGrid *ReadFloatMRMSGrid(char *file) {

  return ReadFloatMRMSGrid(file, NULL);
}

#include "TRMMRTGrid.h"
#include "Messages.h"
#include <cstdio>
#include <zlib.h>

FloatGrid *ReadFloatTRMMRTGrid(char *file, FloatGrid *grid) {

  gzFile fileH;

  fileH = gzopen(file, "rb");
  if (fileH == NULL) {
    return NULL;
  }

  char unprocessed[2880];

  gzread(fileH, unprocessed, 2880);

  if (!grid) {
    grid = new FloatGrid();
    grid->numCols = 1440;
    grid->numRows = 480;
    grid->cellSize = 0.25;
    grid->extent.bottom = -60.0;
    grid->extent.left = -180.0;
    grid->data = new float *[grid->numRows]();
    if (!grid->data) {
      WARNING_LOGF("TRMMRT file %s too large (out of memory) with %li rows",
                   file, grid->numRows);
      delete grid;
      gzclose(fileH);
      return NULL;
    }
    for (long i = 0; i < grid->numRows; i++) {
      grid->data[i] = new float[grid->numCols]();
      if (!grid->data[i]) {
        WARNING_LOGF("TRMMRT file %s too large (out of memory) with %li colums",
                     file, grid->numCols);
        delete grid;
        gzclose(fileH);
        return NULL;
      }
    }
  }
  unsigned short *shortData = new unsigned short[grid->numCols];
  if (!shortData) {
    WARNING_LOGF("TRMMRT file %s too large (out of memory)", file);
    delete grid;
    gzclose(fileH);
    return NULL;
  }

  for (long i = 0; i < grid->numRows; i++) {
    if (gzread(fileH, shortData,
               (unsigned int)(sizeof(short) * grid->numCols)) !=
        (int)sizeof(short) * grid->numCols) {
      WARNING_LOGF("TRMMRT file %s corrupt?", file);
      delete grid;
      delete[] shortData;
      gzclose(fileH);
      return NULL;
    }
    for (int j = 0; j < grid->numCols; j++) {
      int realJ =
          (j >= 720) ? (j - 720) : (j + 720); // Flip this about the Y-axis
      unsigned short realData =
          (shortData[j] >> 8) | ((shortData[j] & 0xFF) << 8);
      float floatData = 0;
      if (realData >= 0 && realData <= 30000) {
        floatData = ((float)realData) / 100.0;
      }
      grid->data[i][realJ] = floatData;
    }
  }

  delete[] shortData;

  // Fill in the rest of the BoundingBox
  grid->extent.top = grid->extent.bottom + grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  gzclose(fileH);

  return grid;
}

FloatGrid *ReadFloatTRMMRTGrid(char *file) {

  return ReadFloatTRMMRTGrid(file, NULL);
}

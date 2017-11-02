#include "TRMMDGrid.h"
#include "Messages.h"
#include <cstdio>
#include <zlib.h>

FloatGrid *ReadFloatTRMMDGrid(char *file, FloatGrid *grid) {

  gzFile fileH;

  fileH = gzopen(file, "rb");
  if (fileH == NULL) {
    return NULL;
  }

  // gzread(fileH, unprocessed, 2880);

  if (!grid) {
    grid = new FloatGrid();
    grid->numCols = 1440;
    grid->numRows = 400;
    grid->cellSize = 0.25;
    grid->extent.bottom = -50.0;
    grid->extent.left = -180.0;
    grid->data = new float *[grid->numRows]();
    if (!grid->data) {
      WARNING_LOGF("TRMM Daily file %s too large (out of memory) with %li rows",
                   file, grid->numRows);
      delete grid;
      gzclose(fileH);
      return NULL;
    }
    for (long i = 0; i < grid->numRows; i++) {
      grid->data[i] = new float[grid->numCols]();
      if (!grid->data[i]) {
        WARNING_LOGF(
            "TRMM Daily file %s too large (out of memory) with %li cols", file,
            grid->numCols);
        delete grid;
        gzclose(fileH);
        return NULL;
      }
    }
  }
  float *shortData = new float[grid->numCols];
  if (shortData) {
    WARNING_LOGF(
        "TRMM Daily file %s too large (out of memory) temporary storage", file);
    delete grid;
    gzclose(fileH);
    return NULL;
  }

  for (long i = 0; i < grid->numRows; i++) {
    if (gzread(fileH, shortData,
               (unsigned int)(sizeof(float) * grid->numCols)) !=
        sizeof(short) * grid->numCols) {
      WARNING_LOGF("TRMM Daily file %s corrupt?", file);
      delete grid;
      delete[] shortData;
      gzclose(fileH);
      return NULL;
    }
    for (int j = 0; j < grid->numCols; j++) {
      int realJ =
          (j > 720) ? (j - 720) : (j + 720); // Flip this about the Y-axis
      unsigned int blah = *(unsigned int *)&(shortData[j]);
      unsigned int bleh = __builtin_bswap32(blah);
      grid->data[grid->numRows - 1 - i][realJ] = *(float *)&bleh;
    }
  }
  delete[] shortData;

  // Fill in the rest of the BoundingBox
  grid->extent.top = grid->extent.bottom + grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  gzclose(fileH);

  return grid;
}

FloatGrid *ReadFloatTRMMDGrid(char *file) {

  return ReadFloatTRMMDGrid(file, NULL);
}

#include "BifGrid.h"
#include "Messages.h"
#include <cstdio>
#include <fcntl.h>

FloatGrid *ReadFloatBifGrid(char *file) {

  BifHeader header;
  FloatGrid *grid = NULL;
  FILE *fileH;

  fileH = fopen(file, "rb");
  if (fileH == NULL) {
    return NULL;
  }

  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_SEQUENTIAL);
  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_WILLNEED);
  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_NOREUSE);

  grid = new FloatGrid();

  if (fread(&header, sizeof(BifHeader), 1, fileH) != 1) {
    WARNING_LOGF("BIF file %s missing header", file);
    fclose(fileH);
    delete grid;
    return NULL;
  }

  grid->numCols = header.ncols;
  grid->numRows = header.nrows;
  grid->cellSize = header.cellsize;
  grid->extent.bottom = header.yllcor;
  grid->extent.left = header.xllcor;

  grid->data = new float *[grid->numRows]();
  if (!grid->data) {
    WARNING_LOGF("BIF file %s too large (out of memory) with %li rows", file,
                 grid->numRows);
    delete grid;
    fclose(fileH);
    return NULL;
  }
  for (long i = 0; i < grid->numRows; i++) {
    grid->data[i] = new float[grid->numCols];
    if (!grid->data[i]) {
      WARNING_LOGF("BIF file %s too large (out of memory) with %li columns",
                   file, grid->numCols);
      delete grid;
      fclose(fileH);
      return NULL;
    }
    if (fread(grid->data[i], sizeof(float), grid->numCols, fileH) !=
        (size_t)grid->numCols) {
      WARNING_LOGF("BIF file %s corrupt?", file);
      delete grid;
      fclose(fileH);
      return NULL;
    }
  }

  // Fill in the rest of the BoundingBox
  grid->extent.top = grid->extent.bottom + grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  fclose(fileH);

  return grid;
}

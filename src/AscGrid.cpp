#include "AscGrid.h"
#include "Messages.h"
#include <cstdio>
#include <fcntl.h>

LongGrid *ReadLongAscGrid(char *file) {

  LongGrid *grid = NULL;
  FILE *fileH;

  fileH = fopen(file, "r");
  if (fileH == NULL) {
    return NULL;
  }

  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_SEQUENTIAL);
  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_WILLNEED);
  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_NOREUSE);

  grid = new LongGrid();

  if (fscanf(fileH, "%*s %ld", &grid->numCols) != 1) {
    delete grid;
    return NULL;
  }
  if (fscanf(fileH, "%*s %ld", &grid->numRows) != 1) {
    delete grid;
    return NULL;
  }
  if (fscanf(fileH, "%*s %lf", &grid->extent.left) != 1) {
    delete grid;
    return NULL;
  }
  if (fscanf(fileH, "%*s %lf", &grid->extent.bottom) != 1) {
    delete grid;
    return NULL;
  }
  if (fscanf(fileH, "%*s %lf", &grid->cellSize) != 1) {
    delete grid;
    return NULL;
  }
  if (fscanf(fileH, "%*s %ld", &grid->noData) != 1) {
    delete grid;
    return NULL;
  }

  grid->data = new long *[grid->numRows];
  for (long i = 0; i < grid->numRows; i++) {
    grid->data[i] = new long[grid->numCols];
  }

  for (long row = 0; row < grid->numRows; row++) {
    for (long col = 0; col < grid->numCols; col++) {
      int c = fscanf(fileH, "%ld", &grid->data[row][col]);
      (void)c;
    }
  }

  // Fill in the rest of the BoundingBox
  grid->extent.top = grid->extent.bottom + grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  fclose(fileH);

  return grid;
}

FloatGrid *ReadFloatAscGrid(char *file) {

  FloatGrid *grid = NULL;

  FILE *fileH;

  fileH = fopen(file, "r");
  if (fileH == NULL) {
    return NULL;
  }

  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_SEQUENTIAL);
  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_WILLNEED);
  // posix_fadvise(fileno(fileH), 0, 0, POSIX_FADV_NOREUSE);

  grid = new FloatGrid();

  if (fscanf(fileH, "%*s %ld", &grid->numCols) != 1) {
    WARNING_LOGF("ASCII file %s missing number of columns", file);
    delete grid;
    fclose(fileH);
    return NULL;
  }
  if (fscanf(fileH, "%*s %ld", &grid->numRows) != 1) {
    WARNING_LOGF("ASCII file %s missing number of rows", file);
    delete grid;
    fclose(fileH);
    return NULL;
  }
  if (fscanf(fileH, "%*s %lf", &grid->extent.left) != 1) {
    WARNING_LOGF("ASCII file %s missing lower left x", file);
    delete grid;
    fclose(fileH);
    return NULL;
  }
  if (fscanf(fileH, "%*s %lf", &grid->extent.bottom) != 1) {
    WARNING_LOGF("ASCII file %s missing lower left y", file);
    delete grid;
    fclose(fileH);
    return NULL;
  }
  if (fscanf(fileH, "%*s %lf", &grid->cellSize) != 1) {
    WARNING_LOGF("ASCII file %s missing cell size", file);
    delete grid;
    fclose(fileH);
    return NULL;
  }
  if (fscanf(fileH, "%*s %f", &grid->noData) != 1) {
    WARNING_LOGF("ASCII file %s missing no data value", file);
    delete grid;
    fclose(fileH);
    return NULL;
  }

  grid->data = new float *[grid->numRows]();
  if (!grid->data) {
    WARNING_LOGF("ASCII file %s too large (out of memory) with %li rows", file,
                 grid->numRows);
    delete grid;
    fclose(fileH);
    return NULL;
  }

  for (long i = 0; i < grid->numRows; i++) {
    grid->data[i] = new float[grid->numCols];
    if (!grid->data[i]) {
      WARNING_LOGF("ASCII file %s too large (out of memory) with %li columns",
                   file, grid->numCols);
      delete grid;
      fclose(fileH);
      return NULL;
    }
  }

  for (long row = 0; row < grid->numRows; row++) {
    for (long col = 0; col < grid->numCols; col++) {
      int c = fscanf(fileH, "%f", &grid->data[row][col]);
      (void)c;
    }
  }

  // Fill in the rest of the BoundingBox
  grid->extent.top = grid->extent.bottom + grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  fclose(fileH);

  return grid;
}

void WriteLongAscGrid(const char *file, LongGrid *grid) {
  FILE *fileH;

  fileH = fopen(file, "w");
  if (fileH == NULL) {
    printf("OMG!\n");
    return;
  }

  // Write out the header info
  fprintf(fileH, "ncols %ld\n", grid->numCols);
  fprintf(fileH, "nrows %ld\n", grid->numRows);
  fprintf(fileH, "xllcorner %f\n", grid->extent.left);
  fprintf(fileH, "yllcorner %f\n", grid->extent.bottom);
  fprintf(fileH, "cellsize %f\n", grid->cellSize);
  fprintf(fileH, "NODATA_value %ld\n", grid->noData);

  // Write out the data
  for (long row = 0; row < grid->numRows; row++) {
    long lastCol = grid->numCols - 1;
    for (long col = 0; col < grid->numCols; col++) {
      fprintf(fileH, "%5ld%s", grid->data[row][col],
              (col == lastCol) ? "\n" : " ");
    }
  }

  fclose(fileH);
}

void WriteFloatAscGrid(const char *file, FloatGrid *grid) {
  FILE *fileH;

  fileH = fopen(file, "w");
  if (fileH == NULL) {
    printf("OMG!\n");
    return;
  }

  // Write out the header info
  fprintf(fileH, "ncols %ld\n", grid->numCols);
  fprintf(fileH, "nrows %ld\n", grid->numRows);
  fprintf(fileH, "xllcorner %f\n", grid->extent.left);
  fprintf(fileH, "yllcorner %f\n", grid->extent.bottom);
  fprintf(fileH, "cellsize %f\n", grid->cellSize);
  fprintf(fileH, "NODATA_value %.02f\n", grid->noData);

  // Write out the data
  for (long row = 0; row < grid->numRows; row++) {
    long lastCol = grid->numCols - 1;
    for (long col = 0; col < grid->numCols; col++) {
      fprintf(fileH, "%.05f%s", grid->data[row][col],
              (col == lastCol) ? "\n" : " ");
    }
  }

  fclose(fileH);
}

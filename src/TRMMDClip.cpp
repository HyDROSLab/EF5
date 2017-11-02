#include <stdio.h>
#include <stdlib.h>

#include "AscGrid.h"
#include "TRMMDGrid.h"

int main(int argc, char *argv[]) {

  if (argc != 3 && argc != 7) {
    printf("Use this program as TRMMDClip <input_filename> <output_filename> "
           "<top> <bottom> <left> <right>\n");
    return 0;
  }

  char *filename = argv[1];
  char *outputfile = argv[2];
  float top = 50.0, bottom = -50.0, left = -180.0, right = 180.0;

  if (argc == 7) {
    top = atof(argv[3]);
    bottom = atof(argv[4]);
    left = atof(argv[5]);
    right = atof(argv[6]);
  }

  int part = top / 0.25;
  top = 0.25 * (float)(part);

  part = bottom / 0.25;
  bottom = 0.25 * (float)(part);

  part = left / 0.25;
  left = 0.25 * (float)(part);

  part = right / 0.25;
  right = 0.25 * (float)(part);

  if (top > 50.0 || top < -50.0 || top <= bottom) {
    printf("Top must be between 60 & -60 and > Bottom\n");
    return 0;
  }

  if (bottom > 50 || bottom < -50) {
    printf("Bottom must be between 60 & -60\n");
    return 0;
  }

  if (left < -180.0 || left > 180.0 || left >= right) {
    printf("Left must be between -180 & 180 and must be < Right\n");
    return 0;
  }

  if (right < -180.0 || right > 180.0) {
    printf("Right must be between -180 & 180\n");
    return 0;
  }

  FloatGrid *outGrid = new FloatGrid;
  outGrid->cellSize = 0.25;
  outGrid->numCols = (right - left) / outGrid->cellSize;
  outGrid->numRows = (top - bottom) / outGrid->cellSize;
  outGrid->extent.bottom = bottom;
  outGrid->extent.left = left;
  outGrid->noData = -999.0;
  outGrid->data = new float *[outGrid->numRows];
  for (long i = 0; i < outGrid->numRows; i++) {
    outGrid->data[i] = new float[outGrid->numCols];
  }
  // Fill in the rest of the BoundingBox
  outGrid->extent.top =
      outGrid->extent.bottom + outGrid->numRows * outGrid->cellSize;
  outGrid->extent.right =
      outGrid->extent.left + outGrid->numCols * outGrid->cellSize;

  FloatGrid *precipGrid = ReadFloatTRMMDGrid(filename);

  if (!precipGrid) {
    printf("Failed to open file %s\n", filename);
    return 0;
  }

  int realTop =
      (precipGrid->extent.top - outGrid->extent.top) / outGrid->cellSize;
  int realLeft =
      (outGrid->extent.left - precipGrid->extent.left) / outGrid->cellSize;

  printf("Real top is %i, left is %i\n", realTop, realLeft);

  for (long y = realTop; y < realTop + outGrid->numRows; y++) {
    for (long x = realLeft; x < realLeft + outGrid->numCols; x++) {
      outGrid->data[y - realTop][x - realLeft] = precipGrid->data[y][x];
    }
  }

  WriteFloatAscGrid(outputfile, outGrid);

  return 1;
}

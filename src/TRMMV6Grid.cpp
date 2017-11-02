#include "TRMMV6Grid.h"
#include <hdf/mfhdf.h>
#include <stdio.h>

union FloatInt {
  unsigned int data;
  float floatVal;
};

void change_endian(unsigned int &x);

void change_endian(unsigned int &val) {
  val = (val << 24) | ((val << 8) & 0x00ff0000) | ((val >> 8) & 0x0000ff00) |
        (val >> 24);
}

FloatGrid *ReadFloatTRMMV6Grid(char *file, FloatGrid *grid) {

  /*FILE *fileH;

  fileH = fopen(file, "rb");
  if (fileH == NULL) {
          return NULL;
  }*/
  // printf("Size is %i\n", sizeof(FloatInt));

  // char unprocessed[298];

  // fread(unprocessed, 298, 1, fileH);

  if (!grid) {
    grid = new FloatGrid();
    grid->numCols = 1440;
    grid->numRows = 400;
    grid->cellSize = 0.25;
    grid->extent.bottom = -50.0;
    grid->extent.left = -180.0;
    grid->data = new float *[grid->numRows];
    grid->noData = -9999.0;
    for (long i = 0; i < grid->numRows; i++) {
      grid->data[i] = new float[grid->numCols];
    }
  }

  int sd_id, sds_id, sds_index;
  // int status;
  int start[3];

  sd_id = SDstart(file, DFACC_READ);
  if (sd_id == -1) {
    printf("Failed to open HDF file!\n");
  }
  sds_index = SDnametoindex(sd_id, "precipitation");
  if (sds_index == -1) {
    printf("Failed to find precipitation field!\n");
  } else {
    // printf("Index is %i\n", sds_index);
  }
  sds_id = SDselect(sd_id, sds_index);
  if (sds_id == -1) {
    printf("Failed to select index!\n");
  } else {
    // printf("SDS id is %i\n", sds_index);
  }
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;
  char sd_name[256];

  int number_type, dimsizes[3], rank, n_attributes;

  SDgetinfo(sds_id, sd_name, &rank, dimsizes, &number_type, &n_attributes);
  // printf("Dim sizes are %i %i %i %s\n", dimsizes[0], dimsizes[1],
  // dimsizes[2], sd_name);

  float testData[1][1440][400];

  SDreaddata(sds_id, start, NULL, dimsizes, testData);
  // printf("Status is %i\n", status);
  SDendaccess(sds_id);
  SDend(sd_id);

  for (long x = 0; x < 1440; x++) {
    for (long y = 0; y < 400; y++) {
      grid->data[399 - y][x] = testData[0][x][y];
    }
  }
  // printf("Got here!\n");

  /*FloatInt *shortData = new FloatInt[grid->numCols];

  for (long i = 0; i < grid->numRows; i++) {
          fread(shortData, sizeof(FloatInt), grid->numCols, fileH);
          for (int j = 0; j < grid->numCols; j++) {
                  change_endian(shortData[j].data);
                  grid->data[i][j] = shortData[j].floatVal;
          }
  }

  delete [] shortData;*/

  // Fill in the rest of the BoundingBox
  grid->extent.top = grid->extent.bottom + grid->numRows * grid->cellSize;
  grid->extent.right = grid->extent.left + grid->numCols * grid->cellSize;

  // fclose(fileH);

  return grid;
}

FloatGrid *ReadFloatTRMMV6Grid(char *file) {

  FloatGrid *grid = NULL;

  return ReadFloatTRMMV6Grid(file, grid);
}

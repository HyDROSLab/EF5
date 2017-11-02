#include <stdio.h>
#include <stdlib.h>

#include "MRMSGrid.h"
#include "TifGrid.h"

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf(
        "Use this program as MRMSConvert <input_filename> <output_filename>\n");
    return 1;
  }

  char *filename = argv[1];
  char *outputfile = argv[2];

  FloatGrid *precipGrid = ReadFloatMRMSGrid(filename);

  if (!precipGrid) {
    printf("Failed to open file %s\n", filename);
    return 1;
  }

  WriteFloatTifGrid(outputfile, precipGrid);

  return 0;
}

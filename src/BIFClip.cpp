#include <stdio.h>
#include <stdlib.h>

#include "AscGrid.h"
#include "BifGrid.h"

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf("Use this program as BIFClip <input_filename> <output_filename>\n");
    return 0;
  }

  char *filename = argv[1];
  char *outputfile = argv[2];

  FloatGrid *precipGrid = ReadFloatBifGrid(filename);

  if (!precipGrid) {
    printf("Failed to open file %s\n", filename);
    return 0;
  }

  WriteFloatAscGrid(outputfile, precipGrid);

  return 1;
}

#include "TifGrid.h"
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

int main(int argc, char **argv) {
  struct dirent *dp;
  DIR *dfd;

  char *dir;
  dir = argv[1];

  if (argc == 1) {
    printf("Usage: %s dirname\n", argv[0]);
    return 0;
  }

  if ((dfd = opendir(dir)) == NULL) {
    fprintf(stderr, "Can't open %s\n", dir);
    return 0;
  }

  std::vector<FloatGrid *> grids;

  char filename_qfd[100];
  char new_name_qfd[100];

  while ((dp = readdir(dfd)) != NULL) {
    struct stat stbuf;
    sprintf(filename_qfd, "%s/%s", dir, dp->d_name);
    if (stat(filename_qfd, &stbuf) == -1) {
      printf("Unable to stat file: %s\n", filename_qfd);
      continue;
    }

    if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
      continue;
      // Skip directories
    } else {

      printf("%s\n", filename_qfd);
      FloatGrid *precipGrid = ReadFloatTifGrid(filename_qfd);
      if (!precipGrid) {
        continue;
      }

      grids.push_back(precipGrid);
    }
  }
  if (grids.size() > 0) {
    FloatGrid *blah = grids[0];
    FloatGrid *avgGrid = new FloatGrid;
    avgGrid->numCols = blah->numCols;
    avgGrid->numRows = blah->numRows;
    avgGrid->cellSize = blah->cellSize;
    avgGrid->extent.top = blah->extent.top;
    avgGrid->extent.bottom = blah->extent.bottom;
    avgGrid->extent.right = blah->extent.right;
    avgGrid->extent.left = blah->extent.left;
    avgGrid->noData = blah->noData;
    avgGrid->data = new float *[avgGrid->numRows];
    FloatGrid *stdGrid = new FloatGrid;
    stdGrid->numCols = blah->numCols;
    stdGrid->numRows = blah->numRows;
    stdGrid->cellSize = blah->cellSize;
    stdGrid->extent.top = blah->extent.top;
    stdGrid->extent.bottom = blah->extent.bottom;
    stdGrid->extent.right = blah->extent.right;
    stdGrid->extent.left = blah->extent.left;
    stdGrid->noData = blah->noData;
    stdGrid->data = new float *[stdGrid->numRows];
    FloatGrid *csGrid = new FloatGrid;
    csGrid->numCols = blah->numCols;
    csGrid->numRows = blah->numRows;
    csGrid->cellSize = blah->cellSize;
    csGrid->extent.top = blah->extent.top;
    csGrid->extent.bottom = blah->extent.bottom;
    csGrid->extent.right = blah->extent.right;
    csGrid->extent.left = blah->extent.left;
    csGrid->noData = blah->noData;
    csGrid->data = new float *[csGrid->numRows];
    for (int i = 0; i < blah->numRows; i++) {
      avgGrid->data[i] = new float[blah->numCols];
      stdGrid->data[i] = new float[blah->numCols];
      csGrid->data[i] = new float[blah->numCols];
    }

    float numYears = (float)(grids.size());

    for (int year = 0; year < grids.size(); year++) {
      for (int j = 0; j < avgGrid->numRows; j++) {
        for (int i = 0; i < avgGrid->numCols; i++) {
          if (year == 0) {
            avgGrid->data[j][i] = 0.0;
            stdGrid->data[j][i] = 0.0;
            csGrid->data[j][i] = 0.0;
          }
          if (grids[year]->data[j][i] == 0.0) {
            grids[year]->data[j][i] = 0.0000001;
          }
          grids[year]->data[j][i] = log10(grids[year]->data[j][i]);
          avgGrid->data[j][i] += (grids[year]->data[j][i] / numYears);
        }
      }
    }

    for (int j = 0; j < avgGrid->numRows; j++) {
      for (int i = 0; i < avgGrid->numCols; i++) {
        float total = 0.0;
        for (int year = 0; year < grids.size(); year++) {
          stdGrid->data[j][i] +=
              powf(grids[year]->data[j][i] - avgGrid->data[j][i], 2.0);
          total += powf(grids[year]->data[j][i] - avgGrid->data[j][i], 3.0);
        }
        stdGrid->data[j][i] /= (numYears - 1.0);
        stdGrid->data[j][i] = sqrt(stdGrid->data[j][i]);
        float csNum = numYears * total;
        float csDom = (numYears - 1.0) * (numYears - 2.0) *
                      powf(stdGrid->data[j][i], 3.0);
        csGrid->data[j][i] = csNum / csDom;
      }
    }

    WriteFloatTifGrid("avgq.tif", avgGrid);
    WriteFloatTifGrid("stdq.tif", stdGrid);
    WriteFloatTifGrid("csq.tif", csGrid);
  }
  return 0;
}

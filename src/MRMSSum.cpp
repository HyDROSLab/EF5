#include <stdio.h>
#include <stdlib.h>

#include "MRMSGrid.h"
#include "TifGrid.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

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

  char filename_qfd[100];
  char new_name_qfd[100];

  FloatGrid *sumGrid = ReadFloatTifGrid("zero.tif");

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
      FloatGrid *precipGrid = ReadFloatMRMSGrid(filename_qfd);
      if (!precipGrid || precipGrid->numRows != sumGrid->numRows ||
          precipGrid->numCols != sumGrid->numCols) {
        if (precipGrid) {
          delete precipGrid;
        }
        continue;
      }
      for (long y = 0; y < sumGrid->numRows; y++) {
        for (long x = 0; x < sumGrid->numCols; x++) {
          if (precipGrid->data[y][x] > 0.0) {
            sumGrid->data[y][x] += (precipGrid->data[y][x] / 12.0);
          }
        }
      }
      delete precipGrid;
    }
  }

  WriteFloatTifGrid("total.tif", sumGrid);
  return 0;
}

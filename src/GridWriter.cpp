#include "GridWriter.h"
#include "BasicConfigSection.h"
#include <climits>

extern LongGrid *g_DEM;

void GridWriter::Initialize(std::vector<GridNode> *nodes) {
  maxX = 0;
  minX = LONG_MAX;
  maxY = 0;
  minY = LONG_MAX;

  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    if (!node->gauge) {
      continue;
    }
    if (node->x > maxX) {
      maxX = node->x;
    }
    if (node->x < minX) {
      minX = node->x;
    }
    if (node->y > maxY) {
      maxY = node->y;
    }
    if (node->y < minY) {
      minY = node->y;
    }
  }

  // We want to be inclusive
  maxX++;
  maxY++;

  long ncols = maxX - minX;
  long nrows = maxY - minY;

  RefLoc ll, ur;

  // Initialize everything in the new grid
  g_DEM->GetRefLoc(minX, maxY, &ll);
  g_DEM->GetRefLoc(maxX, minY, &ur);

  grid.extent.left = ll.x;
  grid.extent.bottom = ll.y;
  grid.extent.right = ur.x;
  grid.extent.top = ur.y;
  grid.numCols = ncols;
  grid.numRows = nrows;
  grid.cellSize = g_DEM->cellSize;
  grid.noData = -9999.0f;

  grid.data = new float *[grid.numRows];
  for (long i = 0; i < grid.numRows; i++) {
    grid.data[i] = new float[grid.numCols];
  }

  // Grid is setup! Copy over no data values everywhere
  for (long row = 0; row < nrows; row++) {
    for (long col = 0; col < ncols; col++) {
      grid.data[row][col] = grid.noData;
    }
  }
}

void GridWriter::WriteGrid(std::vector<GridNode> *nodes,
                           std::vector<float> *data, const char *file,
                           bool ascii) {

  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &(nodes->at(i));
    if (!node->gauge) {
      continue;
    }
    grid.data[node->y - minY][node->x - minX] = data->at(i);
  }

  if (ascii) {
    WriteFloatAscGrid(file, &grid);
  } else {
    char *artist = NULL;
    char *copyright = NULL;
    char *datetime = NULL;
    printf("Artist1 is %s\n", g_basicConfig->GetArtist());
    if ((g_basicConfig->GetArtist())[0]) {
      artist = g_basicConfig->GetArtist();
      printf("Artist is %s\n", artist);
    }
    if ((g_basicConfig->GetCopyright())[0]) {
      copyright = g_basicConfig->GetCopyright();
    }
    WriteFloatTifGrid(file, &grid, artist, datetime, copyright);
  }
}

void GridWriter::WriteGrid(std::vector<GridNode> *nodes,
                           std::vector<double> *data, const char *file,
                           bool ascii) {

  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &(nodes->at(i));
    if (!node->gauge) {
      continue;
    }
    grid.data[node->y - minY][node->x - minX] = data->at(i);
  }

  if (ascii) {
    WriteFloatAscGrid(file, &grid);
  } else {
    char *artist = NULL;
    char *copyright = NULL;
    char *datetime = NULL;
    printf("Artist2 is %s\n", g_basicConfig->GetArtist());
    if (g_basicConfig->GetArtist()[0]) {
      artist = g_basicConfig->GetArtist();
      printf("Artistd is %s\n", artist);
    }
    if (g_basicConfig->GetCopyright()[0]) {
      copyright = g_basicConfig->GetCopyright();
    }
    WriteFloatTifGrid(file, &grid, artist, datetime, copyright);
  }
}

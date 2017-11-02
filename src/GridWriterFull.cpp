#include "GridWriterFull.h"
#include "BasicConfigSection.h"
#include <climits>

extern LongGrid *g_DEM;

void GridWriterFull::Initialize() {
  // Initialize everything in the new grid

  grid.extent.left = g_DEM->extent.left;
  grid.extent.bottom = g_DEM->extent.bottom;
  grid.extent.right = g_DEM->extent.right;
  grid.extent.top = g_DEM->extent.top;
  grid.numCols = g_DEM->numCols;
  grid.numRows = g_DEM->numRows;
  grid.cellSize = g_DEM->cellSize;
  grid.noData = -9999.0f;
  grid.geoSet = g_DEM->geoSet;
  grid.modelType = g_DEM->modelType;
  grid.geographicType = g_DEM->geographicType;
  grid.geodeticDatum = g_DEM->geodeticDatum;

  grid.data = new float *[grid.numRows];
  for (long i = 0; i < grid.numRows; i++) {
    grid.data[i] = new float[grid.numCols];
  }

  // Grid is setup! Copy over no data values everywhere
  for (long row = 0; row < grid.numRows; row++) {
    for (long col = 0; col < grid.numCols; col++) {
      grid.data[row][col] = grid.noData;
    }
  }
}

void GridWriterFull::WriteGrid(std::vector<GridNode> *nodes,
                               std::vector<float> *data, const char *file,
                               bool ascii) {

  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &(nodes->at(i));
    if (!node->gauge) {
      continue;
    }
    grid.data[node->y][node->x] = data->at(i);
  }

  if (ascii) {
    WriteFloatAscGrid(file, &grid);
  } else {
    char *artist = NULL;
    char *copyright = NULL;
    char *datetime = NULL;
    if (g_basicConfig->GetArtist()[0]) {
      artist = g_basicConfig->GetArtist();
    }
    if (g_basicConfig->GetCopyright()[0]) {
      copyright = g_basicConfig->GetCopyright();
    }
    WriteFloatTifGrid(file, &grid, artist, datetime, copyright);
  }
}

void GridWriterFull::WriteGrid(std::vector<GridNode> *nodes,
                               std::vector<double> *data, const char *file,
                               bool ascii) {

  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &(nodes->at(i));
    if (!node->gauge) {
      continue;
    }
    grid.data[node->y][node->x] = data->at(i);
  }

  if (ascii) {
    WriteFloatAscGrid(file, &grid);
  } else {
    char *artist = NULL;
    char *copyright = NULL;
    char *datetime = NULL;
    if (g_basicConfig->GetArtist()[0]) {
      artist = g_basicConfig->GetArtist();
    }
    if (g_basicConfig->GetCopyright()[0]) {
      copyright = g_basicConfig->GetCopyright();
    }
    WriteFloatTifGrid(file, &grid, artist, datetime, copyright);
  }
}

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "AscGrid.h"
#include "BasicGrids.h"
#include "DEMProcessor.h"
#include "Messages.h"
#include "TifGrid.h"

struct DEMNode {
  long x;
  long y;
  float dem;
};

static int ComputeFlowAcc(char *flowAccFile);
static bool SortByHeight(DEMNode *d1, DEMNode *d2);
static bool GetDownstreamNode(long x, long y, long *downX, long *downY);
static bool FlowsInto(DEMNode *d1, DEMNode *d2);

int ProcessDEM(int mode, char *demFile, char *flowDirFile, char *flowAccFile) {
  if (mode == 0) {
    ERROR_LOGF("%s", "Invalid mode option specified");
    return 1;
  }

  if (mode == 2) {
    if (!demFile) {
      ERROR_LOG("No DEM file was specified.");
      return 2;
    } else if (!flowDirFile) {
      ERROR_LOG("No Flow Direction file was specified.");
      return 2;
    } else if (!flowAccFile) {
      ERROR_LOG("No Flow Accumulation file was specified.");
      return 2;
    }

    INFO_LOGF("Loading DEM: %s", demFile);
    char *ext = strrchr(demFile, '.');
    if (!strcasecmp(ext, ".asc")) {
      g_DEM = ReadFloatAscGrid(demFile);
    } else {
      g_DEM = ReadFloatTifGrid(demFile);
    }
    if (!g_DEM) {
      ERROR_LOG("Failed to load DEM!");
      return 2;
    }

    INFO_LOGF("Loading DDM: %s", flowDirFile);
    ext = strrchr(flowDirFile, '.');
    if (!strcasecmp(ext, ".asc")) {
      g_DDM = ReadFloatAscGrid(flowDirFile);
    } else {
      g_DDM = ReadFloatTifGrid(flowDirFile);
    }
    if (!g_DDM) {
      ERROR_LOG("Failed to load DDM!");
      return 2;
    }
    if (!g_DEM->IsSpatialMatch(g_DDM)) {
      ERROR_LOG("The spatial characteristics of the DEM and DDM differ!");
      return 2;
    }

    if (CheckESRIDDM()) {
      ReclassifyDDM();
    }

    ComputeFlowAcc(flowAccFile);
  }

  return 0;
}

int ComputeFlowAcc(char *flowAccFile) {
  std::vector<DEMNode *> nodes;

  g_FAM = new FloatGrid;
  g_FAM->extent.left = g_DEM->extent.left;
  g_FAM->extent.bottom = g_DEM->extent.bottom;
  g_FAM->extent.right = g_DEM->extent.right;
  g_FAM->extent.top = g_DEM->extent.top;
  g_FAM->numCols = g_DEM->numCols;
  g_FAM->numRows = g_DEM->numRows;
  g_FAM->cellSize = g_DEM->cellSize;
  g_FAM->noData = g_DEM->noData;

  g_FAM->data = new float *[g_FAM->numRows];
  for (long i = 0; i < g_FAM->numRows; i++) {
    g_FAM->data[i] = new float[g_FAM->numCols];
  }

  // Grid is setup! Copy over no data values everywhere
  for (long row = 0; row < g_FAM->numRows; row++) {
    for (long col = 0; col < g_FAM->numCols; col++) {
      g_FAM->data[row][col] = g_FAM->noData;
    }
  }

  for (long row = 0; row < g_DEM->numRows; row++) {
    for (long col = 0; col < g_DEM->numCols; col++) {
      if (g_DEM->data[row][col] != g_DEM->noData) {
        DEMNode *node = new DEMNode;
        node->x = col;
        node->y = row;
        node->dem = g_DEM->data[row][col];
        nodes.push_back(node);
      }
    }
  }

  std::stable_sort(nodes.begin(), nodes.end(), SortByHeight);

  for (std::vector<DEMNode *>::iterator itr = nodes.begin(); itr != nodes.end();
       itr++) {
    DEMNode *node = *(itr);
    if (g_FAM->data[node->y][node->x] == g_FAM->noData) {
      g_FAM->data[node->y][node->x] = 0;
    }
    long downX, downY;
    if (GetDownstreamNode(node->x, node->y, &downX, &downY)) {
      if (g_FAM->data[downY][downX] == g_FAM->noData) {
        g_FAM->data[downY][downX] = 0;
      }
      g_FAM->data[downY][downX] =
          g_FAM->data[downY][downX] + g_FAM->data[node->y][node->x] + 1;
      // printf("Processing node z %f, current fac %f, down (%f) is %f (%f)\n",
      // node->dem, g_FAM->data[node->y][node->x], g_DDM->data[node->y][node->x],
      // g_FAM->data[downY][downX], g_DEM->data[downY][downX]);
    }
  }

  WriteFloatTifGrid(flowAccFile, g_FAM);

  return 0;
}

bool GetDownstreamNode(long x, long y, long *downX, long *downY) {
  long nextX = x;
  long nextY = y;

  switch ((int)(g_DDM->data[y][x])) {
  case FLOW_NORTH:
    nextY--;
    break;
  case FLOW_NORTHEAST:
    nextY--;
    nextX++;
    break;
  case FLOW_EAST:
    nextX++;
    break;
  case FLOW_SOUTHEAST:
    nextY++;
    nextX++;
    break;
  case FLOW_SOUTH:
    nextY++;
    break;
  case FLOW_SOUTHWEST:
    nextY++;
    nextX--;
    break;
  case FLOW_WEST:
    nextX--;
    break;
  case FLOW_NORTHWEST:
    nextX--;
    nextY--;
    break;
  default:
    return false;
  }

  if (nextX >= 0 && nextY >= 0 && nextX < g_DEM->numCols &&
      nextY < g_DEM->numRows && g_DEM->data[nextY][nextX] != g_DEM->noData) {
    *downX = nextX;
    *downY = nextY;
    return true;
  }

  return false;
}

bool SortByHeight(DEMNode *d1, DEMNode *d2) {
  if ((d1->x == 115 && d1->y == 55) || (d2->x == 115 && d2->y == 55)) {
    printf("Sorting %i %i (%f) to %i %i (%f)\n", (int)d1->x, (int)d1->y,
           d1->dem, (int)d2->x, (int)d2->y, d2->dem);
  }
  if (d1->x == d2->x && d1->y == d2->y) {
    return false;
  }
  if (fabsf(d1->dem - d2->dem) < 0.10) {
    return FlowsInto(d1, d2);
  } else {
    return d1->dem > d2->dem;
  }
}

bool FlowsInto(DEMNode *d1, DEMNode *d2) {
  long downX, downY;
  long curX = d1->x, curY = d1->y;
  bool print = false;
  if ((d1->x == 115 && d1->y == 55) || (d2->x == 115 && d2->y == 55)) {
    print = true;
  }
  if (print) {
    printf("Comparing %i %i (%f) to %i %i (%f)\n", (int)curX, (int)curY,
           d1->dem, (int)d2->x, (int)d2->y, d2->dem);
  }
  while (GetDownstreamNode(curX, curY, &downX, &downY)) {
    if (d2->x == downX && d2->y == downY) {
      if (print) {
        printf("true\n");
      }
      return true;
    } else if (fabsf(d1->dem - g_DEM->data[downY][downX]) > 0.10) {
      if (print) {
        printf("false\n");
      }
      return false;
    }
    curX = downX;
    curY = downY;
  }
  if (print) {
    printf("false2\n");
  }
  return false;
}

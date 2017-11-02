#include "BasicGrids.h"
#include "AscGrid.h"
#include "BasicConfigSection.h"
#include "Defines.h"
#include "Messages.h"
#include "TifGrid.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <stack>
#include <stdlib.h>

FloatGrid *g_DEM;
FloatGrid *g_DDM;
FloatGrid *g_FAM;
Projection *g_Projection;

struct FAMSearch {
  long x;
  long y;
  bool used;
  long fa;
};

// static FAMSearch *NextUnusedGrid(std::vector<FAMSearch *> *gridCells);
static bool GetDownstreamHeight(long x, long y, long *outsideHeight);
static bool TestUpstream(GridNode *node, FLOW_DIR dir, GridLoc *loc);
static bool TestUpstream(long nextX, long nextY, FLOW_DIR dir, GridLoc *loc);
// static bool TestUpstreamBroken(long nextX, long nextY, FLOW_DIR dir, GridLoc
// *loc);
static GaugeConfigSection *
FindGauge(std::map<unsigned long, GaugeConfigSection *> *gauges,
          GridNode *node);
static GaugeConfigSection *
NextUnusedGauge(std::vector<GaugeConfigSection *> *gauges);
static bool SortByFlowAccumFS(FAMSearch *fs1, FAMSearch *fs2);
static bool SortByFlowAccum(GaugeConfigSection *d1, GaugeConfigSection *d2);
static void FixFAM();
static int FlowsOut(long nextX, long nextY, FLOW_DIR dir, long currentHeight);
static int FlowsIn(long nextX, long nextY, FLOW_DIR dir, long currentHeight,
                   GridLoc *locIn, long maxSearch, GridLoc *locOut);
static void FixFlowDir(BasinConfigSection *basin, std::vector<GridNode> *nodes);
/*FLOW_DIR reverseDir[] = {
  FLOW_SOUTH,
  FLOW_SOUTHWEST,
  FLOW_WEST,
  FLOW_NORTHWEST,
  FLOW_NORTH,
  FLOW_NORTHEAST,
  FLOW_EAST,
  FLOW_SOUTHEAST,
  FLOW_QTY };*/

FLOW_DIR reverseDir[] = {FLOW_WEST,      FLOW_SOUTHWEST, FLOW_SOUTH,
                         FLOW_SOUTHEAST, FLOW_EAST,      FLOW_NORTHEAST,
                         FLOW_NORTH,     FLOW_NORTHWEST, FLOW_QTY};

// This function loads the basic grids and also initializes the projection!
bool LoadBasicGrids() {

  INFO_LOGF("Loading DEM: %s", g_basicConfig->GetDEM());
  char *ext = strrchr(g_basicConfig->GetDEM(), '.');
  if (!strcasecmp(ext, ".asc")) {
    g_DEM = ReadFloatAscGrid(g_basicConfig->GetDEM());
  } else {
    g_DEM = ReadFloatTifGrid(g_basicConfig->GetDEM());
  }
  if (!g_DEM) {
    ERROR_LOG("Failed to load DEM!");
    return false;
  }

  INFO_LOGF("Loading DDM: %s", g_basicConfig->GetDDM());
  ext = strrchr(g_basicConfig->GetDDM(), '.');
  if (!strcasecmp(ext, ".asc")) {
    g_DDM = ReadFloatAscGrid(g_basicConfig->GetDDM());
  } else {
    g_DDM = ReadFloatTifGrid(g_basicConfig->GetDDM());
  }
  if (!g_DDM) {
    ERROR_LOG("Failed to load DDM!");
    return false;
  }
  if (!g_DEM->IsSpatialMatch(g_DDM)) {
    ERROR_LOG("The spatial characteristics of the DEM and DDM differ!");
    return false;
  }

  INFO_LOGF("Loading FAM: %s", g_basicConfig->GetFAM());
  ext = strrchr(g_basicConfig->GetFAM(), '.');
  if (!strcasecmp(ext, ".asc")) {
    g_FAM = ReadFloatAscGrid(g_basicConfig->GetFAM());
  } else {
    g_FAM = ReadFloatTifGrid(g_basicConfig->GetFAM());
  }
  if (!g_FAM) {
    ERROR_LOG("Failed to load FAM!");
    return false;
  }
  if (!g_DEM->IsSpatialMatch(g_FAM)) {
    ERROR_LOG("The spatial characteristics of the DEM and FAM differ!");
    return false;
  }

  if (g_basicConfig->IsESRIDDM()) {
    if (CheckESRIDDM()) {
      ReclassifyDDM();
    } else {
      ERROR_LOG("Was expecting an ESRI Drainage Direction Map and got invalid "
                "values!");
      return false;
    }
  } else {
    if (!CheckSimpleDDM()) {
      ERROR_LOG("Was expecting a simple Drainage Direction Map and got invalid "
                "values!");
      return false;
    }
  }

  FixFAM();

  return true;
}

void FreeBasicGridsData() {
  if (g_DEM && g_DEM->data) {
    for (long i = 0; i < g_DEM->numRows; i++) {
      delete[] g_DEM->data[i];
    }
    delete[] g_DEM->data;
    g_DEM->data = NULL;
  }

  if (g_DDM && g_DDM->data) {
    for (long i = 0; i < g_DDM->numRows; i++) {
      delete[] g_DDM->data[i];
    }
    delete[] g_DDM->data;
    g_DDM->data = NULL;
  }

  if (g_FAM && g_FAM->data) {
    for (long i = 0; i < g_FAM->numRows; i++) {
      delete[] g_FAM->data[i];
    }
    delete[] g_FAM->data;
    g_FAM->data = NULL;
  }
}

void FindIndBasins(float left, float right, float top, float bottom) {

  long maxX = 0;
  long minX = LONG_MAX;
  long maxY = 0;
  long minY = LONG_MAX;
  int totalGauges = 0;

  FloatGrid maskGrid;
  maskGrid.numCols = g_DEM->numCols;
  maskGrid.numRows = g_DEM->numRows;
  maskGrid.extent.top = g_DEM->extent.top;
  maskGrid.extent.bottom = g_DEM->extent.bottom;
  maskGrid.extent.left = g_DEM->extent.left;
  maskGrid.extent.right = g_DEM->extent.right;
  maskGrid.cellSize = g_DEM->cellSize;
  maskGrid.noData = -9999.0;
  maskGrid.data = new float *[maskGrid.numRows];
  for (long i = 0; i < maskGrid.numRows; i++) {
    maskGrid.data[i] = new float[maskGrid.numCols];
    for (long j = 0; j < maskGrid.numCols; j++) {
      maskGrid.data[i][j] = maskGrid.noData;
    }
  }

  printf("Geographic is %f, %f and %f, %f\n", left, right, top, bottom);
  g_Projection->ReprojectPoint(left, bottom, &left, &bottom);
  g_Projection->ReprojectPoint(right, top, &right, &top);
  printf("Projected is %f, %f and %f, %f\n", left, right, top, bottom);

  GridLoc loc1, loc2, locN;
  if (!g_FAM->GetGridLoc(left, bottom, &loc1)) {
    loc1.x = 0;
    loc1.y = g_FAM->numRows;
  }
  if (!g_FAM->GetGridLoc(right, top, &loc2)) {
    loc2.x = g_FAM->numCols;
    loc2.y = 0;
  }

  maxX = g_FAM->numCols; // loc2.x;
  minX = 0;              // loc1.x;
  maxY = g_FAM->numRows; // loc1.y;
  minY = 0;              // loc2.y;

  printf("Min x %li, max x %li, min y %li, max y %li\n", minX, maxX, minY,
         maxY);

  std::list<FAMSearch *> gridCells;
  std::stack<GridLoc *> walkNodes;
  std::vector<std::list<FAMSearch *>::iterator> gcItrs;

  long totalElements = g_DEM->numCols * g_DEM->numRows;
  printf("Total elements in gcItrs is %li\n", totalElements);
  gcItrs.resize(totalElements);
  for (long e = 0; e < totalElements; e++) {
    gcItrs[e] = gridCells.end();
  }

  for (long row = minY; row < maxY; row++) {
    for (long col = minX; col < maxX; col++) {
      long index = row * g_DEM->numCols + col;
      if (g_DEM->data[row][col] == g_DEM->noData ||
          g_FAM->data[row][col] == g_FAM->noData) {
        gcItrs[index] = gridCells.end();
        continue;
      }
      FAMSearch *fs = new FAMSearch;
      fs->x = col;
      fs->y = row;
      fs->fa = g_FAM->data[row][col];
      fs->used = false;
      gridCells.push_front(fs);
      gcItrs[index] = gridCells.begin();
    }
  }

  gridCells.sort(SortByFlowAccumFS);
  FILE *test = fopen("basin_new.txt", "w");
  std::vector<long> gridIndices;

  for (FAMSearch *currentFS = gridCells.front(); currentFS != NULL;
       currentFS = gridCells.front()) {

    if (gridCells.begin() == gridCells.end()) {
      break;
    }

    GridLoc *cCell = new GridLoc;
    cCell->x = currentFS->x;
    cCell->y = currentFS->y;
    maskGrid.data[cCell->y][cCell->x] = totalGauges;
    fprintf(test,
            "[Gauge %i] cellx=%li celly=%li outputts=false #Num Cells = %f\n",
            totalGauges, cCell->x, cCell->y, g_FAM->data[cCell->y][cCell->x]);
    printf("Gauge %i cellx=%li celly=%li basinarea=%f\n", totalGauges, cCell->x,
           cCell->y, g_FAM->data[cCell->y][cCell->x]);
    totalGauges++;

    walkNodes.push(cCell);
    size_t index = cCell->y * g_DEM->numCols + cCell->x;
    if (gcItrs[index] != gridCells.end()) {
      gridCells.erase(gcItrs[index]);
      gcItrs[index] = gridCells.end();
    } else {
      printf("WTF!!!!\n");
    }

    while (!walkNodes.empty()) {
      cCell = walkNodes.top();
      walkNodes.pop();

      maskGrid.data[cCell->y][cCell->x] = totalGauges - 1.0;

      for (int i = 0; i < FLOW_QTY; i++) {
        if (TestUpstream(cCell->x, cCell->y, (FLOW_DIR)i, &locN)) {
          GridLoc *nextN = new GridLoc;
          nextN->x = locN.x;
          nextN->y = locN.y;
          size_t index = nextN->y * g_DEM->numCols + nextN->x;
          if (index < gcItrs.size() &&
              g_DEM->data[nextN->y][nextN->x] != g_DEM->noData &&
              g_FAM->data[nextN->y][nextN->x] != g_FAM->noData) {
            // printf("%i %i %i %i %i %i %i\n", index, nextN->y, nextN->x,
            // g_FAM->data[nextN->y][nextN->x], minY, minX, maxX);
            if (gcItrs[index] != gridCells.end()) {
              gridCells.erase(gcItrs[index]);
            }
            gcItrs[index] = gridCells.end();
            walkNodes.push(nextN);
          } else {
            // printf("Hmm %i (%f, %f, %f)\n", index,
            // g_DEM->data[nextN->y][nextN->x], g_FAM->data[nextN->y][nextN->x],
            // g_DDM->data[nextN->y][nextN->x]);
            delete nextN;
          }
        }
      }

      delete cCell;
    }
  }

  fprintf(test, "[Basin 0]\n");
  for (int i = 0; i < totalGauges; i++) {
    fprintf(test, "gauge=%i ", i);
  }
  fprintf(test, "\n");
  fclose(test);

  WriteFloatTifGrid("maskgrid.tif", &maskGrid);
}

void ClipBasicGrids(long x, long y, long search, const char *output) {

  long maxX = 0;
  long minX = LONG_MAX;
  long maxY = 0;
  long minY = LONG_MAX;

  GridLoc loc;
  loc.x = x;
  loc.y = y;

  maxX = loc.x + search;
  minX = loc.x - search;
  maxY = loc.y + search;
  minY = loc.y - search;

  FindIndBasins(-124.73, -66.95, 50.00, 24.5);
  // FindIndBasins(-84.84, -67.26, 44.17, 35.76);

  printf("Search box is %ld, %ld, %ld, %ld, %ld, %ld\n", minX, maxX, minY, maxY,
         x, y);

  long ncols = maxX - minX;
  long nrows = maxY - minY;

  LongGrid grid;
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
  grid.noData = -9999; // g_DEM->noData;

  grid.data = new long *[grid.numRows];
  for (long i = 0; i < grid.numRows; i++) {
    grid.data[i] = new long[grid.numCols];
  }

  // Grid is setup! Copy over FAM to new grid
  for (long row = minY; row < maxY; row++) {
    for (long col = minX; col < maxX; col++) {
      grid.data[row - minY][col - minX] = g_FAM->data[row][col];
    }
  }

  grid.data[loc.y - minY][loc.x - minX] = -1;

  char buffer[255];
  sprintf(buffer, "%s/%s.clip", output, strrchr(g_basicConfig->GetFAM(), '/'));
  WriteLongAscGrid(buffer, &grid);
}

void ClipBasicGrids(BasinConfigSection *basin, std::vector<GridNode> *nodes,
                    const char *name, const char *output) {

  FixFlowDir(basin, nodes);

  std::vector<GaugeConfigSection *> *gauges = basin->GetGauges();

  long maxX = 0;
  long minX = LONG_MAX;
  long maxY = 0;
  long minY = LONG_MAX;

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

  // printf("Clipping %zd nodes, %lu maxx, %lu minx, %lu maxy, %lu miny\n",
  // nodes->size(), maxX, minX, maxY, minY);

  long ncols = maxX - minX;
  long nrows = maxY - minY;

  FloatGrid grid;
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
  grid.noData = -9999; // g_DEM->noData;

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

  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    if (!node->gauge) {
      continue;
    }
    for (size_t i = 0; i < gauges->size(); i++) {
      GaugeConfigSection *gauge = gauges->at(i);
      if (gauge->GetGridNodeIndex() == (long)node->index) {
        printf("[gauge %s] cellx=%li celly=%li\n", gauge->GetName(),
               node->x - minX, node->y - minY);
        break;
      }
    }
    grid.data[node->y - minY][node->x - minX] = g_DEM->data[node->y][node->x];
  }

  char buffer[255];
  sprintf(buffer, "%s/%s.%s", output, strrchr(g_basicConfig->GetDEM(), '/'),
          name);
  WriteFloatTifGrid(buffer, &grid);

  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    if (!node->gauge) {
      continue;
    }
    grid.data[node->y - minY][node->x - minX] = g_DDM->data[node->y][node->x];
  }

  sprintf(buffer, "%s/%s.%s", output, strrchr(g_basicConfig->GetDDM(), '/'),
          name);
  WriteFloatTifGrid(buffer, &grid);

  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    if (!node->gauge) {
      continue;
    }
    grid.data[node->y - minY][node->x - minX] = g_FAM->data[node->y][node->x];
  }

  sprintf(buffer, "%s/%s.%s", output, strrchr(g_basicConfig->GetFAM(), '/'),
          name);
  WriteFloatTifGrid(buffer, &grid);
}

void FixFlowDir(BasinConfigSection *basin, std::vector<GridNode> *nodes) {
  std::vector<GaugeConfigSection *> *gauges = basin->GetGauges();
  GaugeConfigSection *gauge = gauges->at(0);
  int maxDist = 20000.0 / g_Projection->GetLen(gauge->GetLon(), gauge->GetLat(),
                                               FLOW_NORTH); // 3000;
  if (maxDist < 2) {
    maxDist = 2;
  }

  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    if (node->downStreamNode == INVALID_DOWNSTREAM_NODE) {
      continue;
    }
    GridNode *dsNode = &(nodes->at(node->downStreamNode));
    if (g_DEM->data[node->y][node->x] != g_DEM->data[dsNode->y][dsNode->x]) {
      continue;
    }
    long maxFlow = g_FAM->data[dsNode->y][dsNode->x] * 10000; // maxDist;
    FLOW_DIR flowDir = (FLOW_DIR)g_DDM->data[node->y][node->x];
    long testX, testY;
    float minDist = powf(maxDist, 2.0) + powf(maxDist, 2.0);
    for (int dist = 1; dist < maxDist; dist++) {
      testX = node->x + 1 * dist;
      testY = node->y;
      if (testX < g_FAM->numCols &&
          g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_EAST;
        }
      }

      testX = node->x + 1 * dist;
      testY = node->y - 1 * dist;
      if (testX < g_FAM->numCols && testY >= 0 &&
          g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_NORTHEAST;
        }
      }

      testX = node->x;
      testY = node->y - 1 * dist;
      if (testY >= 0 && g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_NORTH;
        }
      }

      testX = node->x - 1 * dist;
      testY = node->y - 1 * dist;
      if (testX >= 0 && testY >= 0 &&
          g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_NORTHWEST;
        }
      }

      testX = node->x - 1 * dist;
      testY = node->y;
      if (testX >= 0 && g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_WEST;
        }
      }

      testX = node->x - 1 * dist;
      testY = node->y + 1 * dist;
      if (testX >= 0 && testY < g_FAM->numRows &&
          g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_SOUTHWEST;
        }
      }

      testX = node->x;
      testY = node->y + 1 * dist;
      if (testY < g_FAM->numRows &&
          g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_SOUTH;
        }
      }

      testX = node->x + 1 * dist;
      testY = node->y + 1 * dist;
      if (testX < g_FAM->numCols && testY < g_FAM->numRows &&
          g_FAM->data[testY][testX] != g_FAM->noData &&
          g_FAM->data[testY][testX] > maxFlow &&
          g_DEM->data[node->y][node->x] == g_DEM->data[testY][testX]) {
        float thisDist =
            powf(testY - node->y, 2.0) + powf(testX - node->x, 2.0);
        if (thisDist < minDist) {
          minDist = thisDist;
          maxFlow = g_FAM->data[testY][testX];
          flowDir = FLOW_SOUTHEAST;
        }
      }
    }
    if (flowDir != g_DDM->data[node->y][node->x]) {
      printf("Old dir %f, new dir %i\n", g_DDM->data[node->y][node->x],
             flowDir);
      g_DDM->data[node->y][node->x] = (float)(flowDir);
    }
  }
}

void CarveBasin(
    BasinConfigSection *basin, std::vector<GridNode> *nodes,
    std::map<GaugeConfigSection *, float *> *inParamSettings,
    std::map<GaugeConfigSection *, float *> *outParamSettings,
    GaugeMap *gaugeMap, float *defaultParams,
    std::map<GaugeConfigSection *, float *> *inRouteParamSettings,
    std::map<GaugeConfigSection *, float *> *outRouteParamSettings,
    float *defaultRouteParams,
    std::map<GaugeConfigSection *, float *> *inSnowParamSettings,
    std::map<GaugeConfigSection *, float *> *outSnowParamSettings,
    float *defaultSnowParams,
    std::map<GaugeConfigSection *, float *> *inInundationParamSettings,
    std::map<GaugeConfigSection *, float *> *outInundationParamSettings,
    float *defaultInundationParams) {

  std::vector<GaugeConfigSection *> *gauges = basin->GetGauges();
  std::stack<GridNode *> walkNodes;
  size_t currentNode = 0;
  size_t totalAccum = 0;
  GridLoc nextNode;

  // Figure out where each gauge is at and get the flow accumulation from the
  // grid for it. Also resets the used flag to false
  for (std::vector<GaugeConfigSection *>::iterator itr = gauges->begin();
       itr != gauges->end();) {

    GaugeConfigSection *gauge = *(itr);
    GridLoc *loc = gauge->GetGridLoc();
    if (gauge->NeedsProjecting()) {
      if (!g_FAM->GetGridLoc(gauge->GetLon(), gauge->GetLat(), loc)) {
        WARNING_LOGF("Gauge %s is outside the basic grid domain!\n",
                     gauge->GetName());
        itr = gauges->erase(itr);
        continue;
      }
    } else {
      RefLoc ref;
      g_FAM->GetRefLoc(loc->x, loc->y, &ref);
      gauge->SetLat(ref.y);
      gauge->SetLon(ref.x);
    }
    float areaFactor =
        1.0 / g_Projection->GetArea(gauge->GetLon(), gauge->GetLat());
    int maxDist =
        20000.0 / g_Projection->GetLen(gauge->GetLon(), gauge->GetLat(),
                                       FLOW_NORTH); // 3000;
    if (maxDist < 2) {
      maxDist = 2;
    }
    INFO_LOGF("Max gauge search distance is %i", maxDist);
    if (gauge->HasObsFlowAccum()) {
      GridLoc minLoc;
      float testError, minError = pow((float)(g_FAM->data[loc->y][loc->x]) -
                                          gauge->GetObsFlowAccum() * areaFactor,
                                      2.0);
      minLoc.x = loc->x;
      minLoc.y = loc->y;
      long testX, testY;
      for (int dist = 1; dist < maxDist; dist++) {
        testX = loc->x + 1 * dist;
        testY = loc->y;
        if (testX < g_FAM->numCols &&
            g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x + 1 * dist;
        testY = loc->y + 1 * dist;
        if (testX < g_FAM->numCols && testY < g_FAM->numRows &&
            g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x;
        testY = loc->y + 1 * dist;
        if (testY < g_FAM->numRows &&
            g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x - 1 * dist;
        testY = loc->y + 1 * dist;
        if (testX > 0 && testY < g_FAM->numRows &&
            g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x - 1 * dist;
        testY = loc->y;
        if (testX > 0 && g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x - 1 * dist;
        testY = loc->y - 1 * dist;
        if (testX > 0 && testY > 0 &&
            g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x;
        testY = loc->y - 1 * dist;
        if (testY > 0 && g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }

        testX = loc->x + 1 * dist;
        testY = loc->y - 1 * dist;
        if (testX < g_FAM->numCols && testY > 0 &&
            g_FAM->data[testY][testX] != g_FAM->noData) {
          testError = pow((float)(g_FAM->data[testY][testX]) -
                              gauge->GetObsFlowAccum() * areaFactor,
                          2.0);
          if (minError > testError) {
            minError = testError;
            minLoc.x = testX;
            minLoc.y = testY;
          }
        }
      }

      loc->x = minLoc.x;
      loc->y = minLoc.y;
      RefLoc ref;
      g_FAM->GetRefLoc(loc->x, loc->y, &ref);
      gauge->SetLat(ref.y);
      gauge->SetLon(ref.x);
    }
    gauge->SetFlowAccum(g_FAM->data[loc->y][loc->x]);
    gauge->SetUsed(false);
    INFO_LOGF("Gauge %s (%f, %f; %ld, %ld): FAM %ld", gauge->GetName(),
              gauge->GetLat(), gauge->GetLon(), loc->y, loc->x,
              gauge->GetFlowAccum());
    itr++;
  }

  // Sort the gauges into descending order based on flow accumulation
  // This helps us ensure that the independent gauges are the ones at the basin
  // outlets
  std::sort(gauges->begin(), gauges->end(), SortByFlowAccum);

  std::map<unsigned long, GaugeConfigSection *> gaugeCMap;
  for (std::vector<GaugeConfigSection *>::iterator itr = gauges->begin();
       itr != gauges->end();) {
    GaugeConfigSection *gauge = *(itr);
    GridLoc *loc = gauge->GetGridLoc();
    unsigned long index = loc->y * g_DEM->numCols + loc->x;
    std::map<unsigned long, GaugeConfigSection *>::iterator itrM =
        gaugeCMap.find(index);
    if (itrM == gaugeCMap.end()) {
      gaugeCMap.insert(
          std::pair<unsigned long, GaugeConfigSection *>(index, gauge));
      itr++;
    } else {
      WARNING_LOGF("Duplicate gauge!! %s\n", gauge->GetName());
      itr = gauges->erase(itr);
    }
  }

  gaugeMap->Initialize(gauges);

  // Here we compute which grid cells & gauges are upstream of our independent
  // gauges
  for (GaugeConfigSection *currentGauge = (*gauges)[0]; currentGauge != NULL;
       currentGauge = NextUnusedGauge(gauges)) {

    std::map<GaugeConfigSection *, float *>::iterator pitr =
        inParamSettings->find(currentGauge);
    if (pitr == inParamSettings->end() && !defaultParams) {
      ERROR_LOGF(
          "Independent basin \"%s\" lacks a water balance parameter set!",
          currentGauge->GetName());
      return;
    }

    if (pitr == inParamSettings->end()) {
      outParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
          currentGauge, defaultParams));
    } else {
      outParamSettings->insert(
          std::pair<GaugeConfigSection *, float *>(pitr->first, pitr->second));
    }

    if (inRouteParamSettings) {
      pitr = inRouteParamSettings->find(currentGauge);
      if (pitr == inRouteParamSettings->end() && !defaultRouteParams) {
        ERROR_LOGF("Independent basin \"%s\" lacks a routing parameter set!",
                   currentGauge->GetName());
        return;
      }

      if (pitr == inRouteParamSettings->end()) {
        outRouteParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
            currentGauge, defaultRouteParams));
      } else {
        outRouteParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
            pitr->first, pitr->second));
      }
    }

    if (inSnowParamSettings) {
      pitr = inSnowParamSettings->find(currentGauge);
      if (pitr == inSnowParamSettings->end() && !defaultSnowParams) {
        ERROR_LOGF("Independent basin \"%s\" lacks a snow parameter set!",
                   currentGauge->GetName());
        return;
      }

      if (pitr == inSnowParamSettings->end()) {
        outSnowParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
            currentGauge, defaultSnowParams));
      } else {
        outSnowParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
            pitr->first, pitr->second));
      }
    }

    if (inInundationParamSettings) {
      pitr = inInundationParamSettings->find(currentGauge);
      if (pitr == inInundationParamSettings->end() &&
          !defaultInundationParams) {
        ERROR_LOGF("Independent basin \"%s\" lacks a inundation parameter set!",
                   currentGauge->GetName());
        return;
      }

      if (pitr == inInundationParamSettings->end()) {
        outInundationParamSettings->insert(
            std::pair<GaugeConfigSection *, float *>(currentGauge,
                                                     defaultInundationParams));
      } else {
        outInundationParamSettings->insert(
            std::pair<GaugeConfigSection *, float *>(pitr->first,
                                                     pitr->second));
      }
    }

    if (g_basicConfig->IsSelfFAM()) {
      totalAccum += currentGauge->GetFlowAccum();
    } else {
      totalAccum += (currentGauge->GetFlowAccum() + 1);
    }
    nodes->resize(totalAccum);
    GridNode *currentN = &(*nodes)[currentNode];

    // Setup the initial node for initiating the search for upstream nodes
    currentN->index = currentNode;
    currentNode++;
    currentN->x = currentGauge->GetGridLoc()->x;
    currentN->y = currentGauge->GetGridLoc()->y;
    g_DEM->GetRefLoc(currentN->x, currentN->y, &currentN->refLoc);
    if (g_DEM->data[currentN->y][currentN->x] == g_DEM->noData ||
        g_FAM->data[currentN->y][currentN->x] == g_FAM->noData ||
        g_FAM->data[currentN->y][currentN->x] < 0) {
      ERROR_LOGF("Gauge \"%s\" is located in a no data grid cell!",
                 currentGauge->GetName());
      return;
    }
    currentN->downStreamNode = INVALID_DOWNSTREAM_NODE;
    long outsideHeight = 0;
    if (GetDownstreamHeight(currentN->x, currentN->y, &outsideHeight)) {
      currentN->horLen =
          g_Projection->GetLen(currentN->refLoc.x, currentN->refLoc.y,
                               (FLOW_DIR)g_DDM->data[currentN->y][currentN->x]);
      float DEMDiff = g_DEM->data[currentN->y][currentN->x] - outsideHeight;
      currentN->slope = ((DEMDiff < 1.0) ? 1.0 : DEMDiff) / currentN->horLen;
    } else {
      currentN->horLen =
          g_Projection->GetLen(currentN->refLoc.x, currentN->refLoc.y,
                               FLOW_NORTH); // We assume a horizontal length
                                            // because we know nothing further
      currentN->slope = 1.0 / currentN->horLen; // We assume a difference in
                                                // height of 1 meter because we
                                                // know nothing else
    }
    currentN->area =
        g_Projection->GetArea(currentN->refLoc.x, currentN->refLoc.y);
    currentN->contribArea = currentN->area;
    currentN->fac = g_FAM->data[currentN->y][currentN->x];
    walkNodes.push(currentN);

    while (!walkNodes.empty()) {

      // Get the next node to check off the stack
      currentN = walkNodes.top();
      walkNodes.pop();

      // Store the previous gauge
      GaugeConfigSection *prevGauge = currentN->gauge;

      // Is this node actually a gauge?
      GaugeConfigSection *nodeGauge = FindGauge(&gaugeCMap, currentN);
      bool keepGoing = true;
      if (nodeGauge) {
        nodeGauge->SetGridNodeIndex(currentN->index);
        currentN->gauge = nodeGauge;
        nodeGauge->SetUsed(true);
        keepGoing = nodeGauge->ContinueUpstream();

        // Add this to the gauge map which allows us to figure out upstream
        // gauges & contributions
        gaugeMap->AddUpstreamGauge(prevGauge, nodeGauge);

        // We fill in outParamSettings for the new gauge here
        // Since it is not required that inParamSettings contain parameters for
        // every gauge in the basin we will in parameters from downstream gauges
        // upstream
        pitr = inParamSettings->find(nodeGauge);
        if (pitr == inParamSettings->end()) {
          pitr = outParamSettings->find(prevGauge);
          outParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
              nodeGauge, pitr->second));
        } else {
          outParamSettings->insert(std::pair<GaugeConfigSection *, float *>(
              pitr->first, pitr->second));
        }
        if (inRouteParamSettings) {
          pitr = inRouteParamSettings->find(nodeGauge);
          if (pitr == inRouteParamSettings->end()) {
            pitr = outRouteParamSettings->find(prevGauge);
            outRouteParamSettings->insert(
                std::pair<GaugeConfigSection *, float *>(nodeGauge,
                                                         pitr->second));
          } else {
            outRouteParamSettings->insert(
                std::pair<GaugeConfigSection *, float *>(pitr->first,
                                                         pitr->second));
          }
        }

        if (inSnowParamSettings) {
          pitr = inSnowParamSettings->find(nodeGauge);
          if (pitr == inSnowParamSettings->end()) {
            pitr = outSnowParamSettings->find(prevGauge);
            outSnowParamSettings->insert(
                std::pair<GaugeConfigSection *, float *>(nodeGauge,
                                                         pitr->second));
          } else {
            outSnowParamSettings->insert(
                std::pair<GaugeConfigSection *, float *>(pitr->first,
                                                         pitr->second));
          }
        }

        if (inInundationParamSettings) {
          pitr = inInundationParamSettings->find(nodeGauge);
          if (pitr == inInundationParamSettings->end()) {
            pitr = outInundationParamSettings->find(prevGauge);
            outInundationParamSettings->insert(
                std::pair<GaugeConfigSection *, float *>(nodeGauge,
                                                         pitr->second));
          } else {
            outInundationParamSettings->insert(
                std::pair<GaugeConfigSection *, float *>(pitr->first,
                                                         pitr->second));
          }
        }
      }

      long currentNDEM = g_DEM->data[currentN->y][currentN->x];

      // Lets figure out what flows into this node
      // We compute slope here too!
      if (keepGoing) {
        for (int i = 1; i < FLOW_QTY; i++) {
          if (TestUpstream(currentN, (FLOW_DIR)i, &nextNode)) {
            GridNode *nextN = &(*nodes)[currentNode];
            nextN->index = currentNode;
            currentNode++;
            nextN->x = nextNode.x;
            nextN->y = nextNode.y;
            nextN->downStreamNode = currentN->index;
            nextN->gauge = currentN->gauge;
            nextN->fac = g_FAM->data[nextN->y][nextN->x];

            // Calculate slope!
            long nextNDEM = g_DEM->data[nextN->y][nextN->x];
            float DEMDiff =
                (float)(nextNDEM - currentNDEM); // Upstream (higher elevation)
                                                 // minus downstream (lower
                                                 // elevation)
            g_DEM->GetRefLoc(nextN->x, nextN->y, &nextN->refLoc);
            nextN->horLen = g_Projection->GetLen(nextN->refLoc.x,
                                                 nextN->refLoc.y, (FLOW_DIR)i);
            nextN->slope = ((DEMDiff < 1.0) ? 1.0 : DEMDiff) / nextN->horLen;
            nextN->area =
                g_Projection->GetArea(nextN->refLoc.x, nextN->refLoc.y);
            nextN->contribArea = nextN->area;
            // printf("Pushing node %i %i (%i, %i) from %i %i (%i, %i) %i %i\n",
            // nextN->x, nextN->y, g_DDM->data[nextN->y][nextN->x],
            // g_FAM->data[nextN->y][nextN->x], currentN->x, currentN->y,
            // g_DDM->data[currentN->y][currentN->x],
            // g_FAM->data[currentN->y][currentN->x], currentNode,
            // nodes->size());
            walkNodes.push(nextN);
          }
        }
      }
    }

    INFO_LOGF("Walked %lu (out of %lu) nodes for %s!",
              (unsigned long)currentNode, (unsigned long)totalAccum,
              currentGauge->GetName());
  }

  nodes->resize(currentNode);

  for (long i = nodes->size() - 1; i >= 0; i--) {
    GridNode *node = &nodes->at(i);
    if (node->downStreamNode != INVALID_DOWNSTREAM_NODE) {
      nodes->at(node->downStreamNode).contribArea += node->contribArea;
    }
  }

  /*FILE *fp = fopen("gauge_data.txt", "w");
  FILE *fp2 = fopen("basin_data.txt", "w");
  for (std::vector<GaugeConfigSection *>::iterator itr = gauges->begin(); itr !=
  gauges->end(); itr++ ) { GaugeConfigSection *gauge = *(itr); GridLoc *loc =
  gauge->GetGridLoc(); fprintf(fp, "[Gauge %s] cellx=%li celly=%li basinarea=%f
  %ld %li wantco=true outputts=false\n", gauge->GetName(), loc->x, loc->y,
  nodes->at(gauge->GetGridNodeIndex()).contribArea, gauge->GetFlowAccum(),
  gauge->GetGridNodeIndex()); fprintf(fp2, "gauge=%s\n", gauge->GetName());
  }
  fclose(fp);
  fclose(fp2);
  */
}

bool GetDownstreamHeight(long x, long y, long *outsideHeight) {
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
    *outsideHeight = g_DEM->data[nextY][nextX];
    return true;
  }

  return false;
}

bool TestUpstream(GridNode *node, FLOW_DIR dir, GridLoc *loc) {
  return TestUpstream(node->x, node->y, dir, loc);
}

bool TestUpstream(long nextX, long nextY, FLOW_DIR dir, GridLoc *loc) {
  FLOW_DIR wantDir;

  // unsigned long currentFAC = g_FAM->data[nextY][nextX];

  switch (dir) {
  case FLOW_NORTH:
    nextY--;
    wantDir = FLOW_SOUTH;
    break;
  case FLOW_NORTHEAST:
    nextY--;
    nextX++;
    wantDir = FLOW_SOUTHWEST;
    break;
  case FLOW_EAST:
    nextX++;
    wantDir = FLOW_WEST;
    break;
  case FLOW_SOUTHEAST:
    nextY++;
    nextX++;
    wantDir = FLOW_NORTHWEST;
    break;
  case FLOW_SOUTH:
    nextY++;
    wantDir = FLOW_NORTH;
    break;
  case FLOW_SOUTHWEST:
    nextY++;
    nextX--;
    wantDir = FLOW_NORTHEAST;
    break;
  case FLOW_WEST:
    nextX--;
    wantDir = FLOW_EAST;
    break;
  case FLOW_NORTHWEST:
    nextX--;
    nextY--;
    wantDir = FLOW_SOUTHEAST;
    break;
  default:
    return false;
  }

  if (nextX >= 0 && nextY >= 0 && nextX < g_DDM->numCols &&
      nextY < g_DDM->numRows &&
      g_DDM->data[nextY][nextX] ==
          wantDir) { // && g_FAM->data[nextY][nextX] <= currentFAC) {
    loc->x = nextX;
    loc->y = nextY;
    return true;
  }

  return false;
}

#if 0
bool TestUpstreamBroken(long nextX, long nextY, FLOW_DIR dir, GridLoc *loc) {
  FLOW_DIR wantDir;
  
  unsigned long currentFAC = g_FAM->data[nextY][nextX];
  
  switch (dir) {
    case FLOW_NORTH:
      nextY--;
      wantDir = FLOW_SOUTH;
      break;
    case FLOW_NORTHEAST:
      nextY--;
      nextX++;
      wantDir = FLOW_SOUTHWEST;
      break;
    case FLOW_EAST:
      nextX++;
      wantDir = FLOW_WEST;
      break;
    case FLOW_SOUTHEAST:
      nextY++;
      nextX++;
      wantDir = FLOW_NORTHWEST;
      break;
    case FLOW_SOUTH:
      nextY++;
      wantDir = FLOW_NORTH;
      break;
    case FLOW_SOUTHWEST:
      nextY++;
      nextX--;
      wantDir = FLOW_NORTHEAST;
      break;
    case FLOW_WEST:
      nextX--;
      wantDir = FLOW_EAST;
      break;
		case FLOW_NORTHWEST:
      nextX--;
      nextY--;
      wantDir = FLOW_SOUTHEAST;
      break;
    default:
      return false;
  }
  
  if (nextX >= 0 && nextY >= 0 && nextX < g_DDM->numCols && nextY < g_DDM->numRows && g_DDM->data[nextY][nextX] == wantDir && g_FAM->data[nextY][nextX] > currentFAC) {
    loc->x = nextX;
    loc->y = nextY;
    return true;
  }
  
  return false;
}
#endif

GaugeConfigSection *
FindGauge(std::map<unsigned long, GaugeConfigSection *> *gauges,
          GridNode *node) {

  unsigned long index = node->y * g_DEM->numCols + node->x;

  std::map<unsigned long, GaugeConfigSection *>::iterator itr =
      gauges->find(index);

  if (itr == gauges->end()) {
    return NULL;
  } else {
    return itr->second;
  }

  /*for (std::vector<GaugeConfigSection *>::iterator itr = gauges->begin(); itr
!= gauges->end(); itr++) {

GaugeConfigSection *gauge = *(itr);
GridLoc *loc = gauge->GetGridLoc();
if (loc->x == node->x && loc->y == node->y) {
printf("Next gauge is %s\n", gauge->GetName());
return gauge;
}
}

return NULL;*/
}

GaugeConfigSection *NextUnusedGauge(std::vector<GaugeConfigSection *> *gauges) {

  for (std::vector<GaugeConfigSection *>::iterator itr = gauges->begin();
       itr != gauges->end(); itr++) {

    GaugeConfigSection *gauge = *(itr);
    if (!gauge->GetUsed()) {
      return gauge;
    }
  }

  return NULL;
}

bool SortByFlowAccum(GaugeConfigSection *d1, GaugeConfigSection *d2) {
  return d1->GetFlowAccum() > d2->GetFlowAccum();
}

bool SortByFlowAccumFS(FAMSearch *fs1, FAMSearch *fs2) {
  return fs1->fa > fs2->fa;
}

bool CheckESRIDDM() {
  for (long row = 0; row < g_DDM->numRows; row++) {
    for (long col = 0; col < g_DDM->numCols; col++) {
      if (g_DDM->data[row][col] == g_DDM->noData) {
        continue;
      }
      switch ((int)(g_DDM->data[row][col])) {
      case 0:
        g_DDM->data[row][col] = g_DDM->noData;
      case 1:
      case 2:
      case 4:
      case 8:
      case 16:
      case 32:
      case 64:
      case 128:
        continue;
      default:
        ERROR_LOGF("Bad DDM value %i at (%li, %li) %f",
                   (int)(g_DDM->data[row][col]), col, row, g_DDM->noData);
        return false;
      }
    }
  }
  return true;
}

bool CheckSimpleDDM() {
  for (long row = 0; row < g_DDM->numRows; row++) {
    for (long col = 0; col < g_DDM->numCols; col++) {
      if (g_DDM->data[row][col] == g_DDM->noData) {
        continue;
      }
      switch ((int)(g_DDM->data[row][col])) {
      case 0:
        g_DDM->data[row][col] = g_DDM->noData;
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        continue;
      default:
        return false;
      }
    }
  }
  return true;
}

void ReclassifyDDM() {

  for (long row = 0; row < g_DDM->numRows; row++) {
    for (long col = 0; col < g_DDM->numCols; col++) {
      switch ((int)(g_DDM->data[row][col])) {
      case 64:
        g_DDM->data[row][col] = FLOW_NORTH;
        break;
      case 128:
        g_DDM->data[row][col] = FLOW_NORTHEAST;
        break;
      case 1:
        g_DDM->data[row][col] = FLOW_EAST;
        break;
      case 2:
        g_DDM->data[row][col] = FLOW_SOUTHEAST;
        break;
      case 4:
        g_DDM->data[row][col] = FLOW_SOUTH;
        break;
      case 8:
        g_DDM->data[row][col] = FLOW_SOUTHWEST;
        break;
      case 16:
        g_DDM->data[row][col] = FLOW_WEST;
        break;
      case 32:
        g_DDM->data[row][col] = FLOW_NORTHWEST;
        break;
      }
    }
  }
}

void FixFAM() {
  // GridLoc locN;

  for (long row = 0; row < g_DEM->numRows; row++) {
    for (long col = 0; col < g_DEM->numCols; col++) {
      if (g_DEM->data[row][col] == g_DEM->noData ||
          g_FAM->data[row][col] == g_FAM->noData ||
          g_DDM->data[row][col] == g_DDM->noData) {
        g_DEM->data[row][col] = g_DEM->noData;
        g_FAM->data[row][col] = g_FAM->noData;
        g_DDM->data[row][col] = g_DDM->noData;
      }
    }
  }

  return;

  /*
        for (long row = 0; row < g_FAM->numRows; row++) {
    for (long col = 0; col < g_FAM->numCols; col++) {
                        if (g_FAM->data[row][col] != g_FAM->noData) {
                                for (int i = 1; i < FLOW_QTY; i++) {
          if (TestUpstreamBroken(col, row, (FLOW_DIR)i, &locN)) {
                                                INFO_LOGF("Adding %f cells to
    accumulation for cell (%li, %li; %f)\n", g_FAM->data[row][col], col, row,
    g_FAM->data[locN.y][locN.x]); g_FAM->data[locN.y][locN.x] +=
    g_FAM->data[row][col]; if (!g_basicConfig->IsSelfFAM()) {
                                                        g_FAM->data[locN.y][locN.x]
    += 1;
                                                }
                                        }
                                }
                        }
                }

        }*/
}

int FlowsOut(long nextX, long nextY, FLOW_DIR dir, long currentHeight) {

  switch (dir) {
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
    return 0; // Doesn't flow out
  }

  if (nextX >= 0 && nextY >= 0 && nextX < g_DEM->numCols &&
      nextY < g_DEM->numRows) {
    if (g_DEM->data[nextY][nextX] == g_DEM->noData) {
      return 1; // Flows out
    }

    if (g_DEM->data[nextY][nextX] <= currentHeight) {
      return 1; // Flows out
    }

    if (g_DEM->data[nextY][nextX] > currentHeight) {
      return 2; // Flows in, not out
    }
  }

  return 0; // Doesn't flow out
}

int FlowsIn(long nextX, long nextY, FLOW_DIR dir, long currentHeight,
            GridLoc *locIn, long maxSearch, GridLoc *locOut) {

  switch (dir) {
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
    return 0; // Doesn't flow in
  }

  if (nextX >= 0 && nextY >= 0 && nextX < g_DEM->numCols &&
      nextY < g_DEM->numRows) {
    if (labs(nextX - locIn->x) > maxSearch ||
        labs(nextY - locIn->y) > maxSearch) {
      return 0; // To Far away, don't consider it.
    }

    if (g_DEM->data[nextY][nextX] == g_DEM->noData) {
      return 0; // Doesn't flow in
    }

    if (g_DEM->data[nextY][nextX] < currentHeight) {
      return 0; // Flows out
    }

    if (g_DEM->data[nextY][nextX] >= currentHeight) {
      locOut->x = nextX;
      locOut->y = nextY;
      return 1; // Flows in
    }
  }

  return 0; // Doesn't flow out
}

void MakeBasic() {
  int totalSinks = 0;
  for (long row = 0; row < g_DEM->numRows; row++) {
    for (long col = 0; col < g_DEM->numCols; col++) {
      if (g_DEM->data[row][col] == g_DEM->noData) {
        continue;
      }
      bool flowsIn = false;
      bool flowsOut = false;
      for (int i = 1; i < FLOW_QTY; i++) {
        int result = FlowsOut(col, row, (FLOW_DIR)i, g_DEM->data[row][col]);
        if (result == 2) {
          flowsIn = true;
        } else if (result == 1) {
          flowsOut = true;
          break;
        }
      }
      if (!flowsOut && flowsIn) {
        totalSinks++;
        std::stack<GridNode *> walkNodes;
        std::vector<GridNode *> sinkNodes;
        GridNode *currentN = new GridNode;
        GridLoc locIn, locOut;
        locIn.x = col;
        locIn.y = row;
        currentN->x = col;
        currentN->y = row;
        currentN->fac = -1;
        walkNodes.push(currentN);
        while (!walkNodes.empty()) {
          // Get the next node to check off the stack
          currentN = walkNodes.top();
          walkNodes.pop();
          sinkNodes.push_back(currentN);
          for (int i = 1; i < FLOW_QTY; i++) {
            if (i == currentN->fac) {
              continue;
            }
            if (FlowsIn(currentN->x, currentN->y, (FLOW_DIR)i,
                        g_DEM->data[currentN->y][currentN->x], &locIn, 5,
                        &locOut)) {
              GridNode *newN = new GridNode;
              newN->x = locOut.x;
              newN->y = locOut.y;
              newN->fac = reverseDir[i];
            }
          }
        }

        // if (col >= 1 && row >= 1)
        // printf("Found inflow sink %i %i %i (%i %i %i %i %i %i %i %i)!\n",
        // col, row, g_DEM->data[row][col], g_DEM->data[row-1][col-1],
        // g_DEM->data[row-1][col], g_DEM->data[row-1][col+1],
        // g_DEM->data[row][col-1], g_DEM->data[row][col+1],
        // g_DEM->data[row+1][col-1], g_DEM->data[row+1][col],
        // g_DEM->data[row+1][col+1]);
      }
    }
  }

  printf("Total sinks %i", totalSinks);
}

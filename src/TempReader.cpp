#include "TempReader.h"
#include "AscGrid.h"
#include "BifGrid.h"
#include "Messages.h"
#include "TifGrid.h"
#include <cmath>
#include <cstdio>
#include <cstring>

void TempReader::ReadDEM(char *file) {
  tempDEM = ReadFloatTifGrid(file);
  if (!tempDEM) {
    WARNING_LOGF("Failed to load temperature grid DEM %s\n", file);
  } else {
    INFO_LOGF("Successfully loaded temperature grid DEM %s\n", file);
  }
}

bool TempReader::Read(char *file, SUPPORTED_TEMP_TYPES type,
                      std::vector<GridNode> *nodes,
                      std::vector<float> *currentTemp,
                      std::vector<float> *prevTemp, bool hasF) {
  if (!strcmp(lastTempFile, file)) {
    if (prevTemp) {
      for (size_t i = 0; i < nodes->size(); i++) {
        currentTemp->at(i) = prevTemp->at(i);
      }
    }
    return true; // This is the same temp file that we read last time, we assume
                 // currentPET is still valid!
  }

  if (!hasF) {
    // Update this here so we recheck for missing files & don't recheck for
    // forecast precip
    strcpy(lastTempFile, file);
  }

  FloatGrid *tempGrid = NULL;

  switch (type) {
  case TEMP_ASCII:
    tempGrid = ReadFloatAscGrid(file);
    break;
  case TEMP_TIF:
    tempGrid = ReadFloatTifGrid(file);
    break;
  default:
    ERROR_LOG("Unsupported Temp format!");
    break;
  }

  if (!tempGrid) {
    // The temp file was not found! We return zeros if there is no qpf.
    if (!hasF) {
      for (size_t i = 0; i < nodes->size(); i++) {
        currentTemp->at(i) = 0;
      }
    }
    return false;
  }

  if (hasF) {
    // Update this here so we recheck for missing files & don't recheck for
    // forecast precip
    strcpy(lastTempFile, file);
  }

  // We have two options now... Either the temp grid & the basic grids are the
  // same Or they are different!

  if (g_DEM->IsSpatialMatch(tempGrid)) {
    // The grids are the same! Our life is easy!
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (tempGrid->data[node->y][node->x] != tempGrid->noData) {
        currentTemp->at(i) = tempGrid->data[node->y][node->x];
      } else {
        currentTemp->at(i) = 0.0;
      }
    }

  } else {
    // The grids are different, we must do some resampling fun.
    GridLoc pt;
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (tempGrid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) &&
          tempGrid->data[pt.y][pt.x] != tempGrid->noData) {
        if (tempDEM && tempDEM->IsSpatialMatch(tempGrid)) {
          float temp = tempGrid->data[pt.y][pt.x];
          float diffHeight =
              g_DEM->data[node->y][node->x] - tempDEM->data[pt.y][pt.x];
          float tempMod = -0.0065 * diffHeight;
          currentTemp->at(i) = temp + tempMod;
        } else {
          currentTemp->at(i) = tempGrid->data[pt.y][pt.x];
        }
      } else {
        currentTemp->at(i) = 0.0;
      }
    }
  }

  // We don't actually need to keep the PET grid in memory anymore
  delete tempGrid;

  return true;
}

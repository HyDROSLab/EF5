#include "PETReader.h"
#include "AscGrid.h"
#include "BifGrid.h"
#include "Messages.h"
#include "TifGrid.h"
#include <cmath>
#include <cstdio>
#include <cstring>

bool PETReader::Read(char *file, SUPPORTED_PET_TYPES type,
                     std::vector<GridNode> *nodes,
                     std::vector<float> *currentPET, float petConvert,
                     bool isTemp, float jday, std::vector<float> *prevPET) {
  if (!strcmp(lastPETFile, file)) {
    if (prevPET) {
      for (size_t i = 0; i < nodes->size(); i++) {
        currentPET->at(i) = prevPET->at(i);
      }
    }
    return true; // This is the same pet file that we read last time, we assume
                 // currentPET is still valid!
  }

  strcpy(lastPETFile, file);

  FloatGrid *petGrid = NULL;

  switch (type) {
  case PET_ASCII:
    petGrid = ReadFloatAscGrid(file);
    break;
  case PET_BIF:
    petGrid = ReadFloatBifGrid(file);
    break;
  case PET_TIF:
    petGrid = ReadFloatTifGrid(file);
    break;
  default:
    ERROR_LOG("Unsupported PET format!");
    break;
  }

  if (!petGrid) {
    // If the file is not found or something else is wrong we assume zero values
    for (size_t i = 0; i < nodes->size(); i++) {
      currentPET->at(i) = 0;
    }
    return false;
  }

  // We have two options now... Either the pet grid & the basic grids are the
  // same Or they are different!

  if (g_DEM->IsSpatialMatch(petGrid)) {
    // The grids are the same! Our life is easy!
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (petGrid->data[node->y][node->x] > 0.0) {
        currentPET->at(i) = petGrid->data[node->y][node->x] * petConvert;
      } else {
        currentPET->at(i) = 0.0;
      }
    }

  } else {
    // The grids are different, we must do some resampling fun.
    GridLoc pt;
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (petGrid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) &&
          petGrid->data[pt.y][pt.x] > 0.0) {
        currentPET->at(i) = petGrid->data[pt.y][pt.x] * petConvert;
      } else {
        currentPET->at(i) = 0;
      }
    }
  }

  // See if this is a temperature grid and if so convert it into PET using Hamon
  // (1961).
  if (isTemp) {
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (currentPET->at(i) <= 0 && currentPET->at(i) != petGrid->noData) {
        currentPET->at(i) =
            0; // Hey, its below freezing, no potential evaporation!
      } else if (currentPET->at(i) != petGrid->noData) {
        float lon, lat;
        RefLoc pt;
        g_DEM->GetRefLoc(node->x, node->y, &pt);
        g_Projection->UnprojectPoint(pt.x, pt.y, &lon, &lat);
        float e_s = 0.2749e8 * exp(-4278.6 / (currentPET->at(i) + 242.8));
        float delta = 0.4093 * sin(2 * PI * jday / 365 - 1.405);
        float omega_s = acos(-tan(TORADIANS(lat)) * tan(delta));
        float H_t = 24 * omega_s / PI;
        float E_t = 2.1 * pow(H_t, 2) * e_s / (currentPET->at(i) + 273.3);
        // printf("(%f, %f) %f, %f, %f, %f, %f, %f\n", lat, lon,
        // currentPET->at(i), e_s, delta, omega_s, H_t, E_t);
        currentPET->at(i) =
            E_t / 24; // These E_t values are mm day ^ -1, we want mm h ^ -1
      }
    }
  }

  // We don't actually need to keep the PET grid in memory anymore
  delete petGrid;

  return true;
}

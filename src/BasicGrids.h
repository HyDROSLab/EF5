#ifndef BASIC_GRIDS_H
#define BASIC_GRIDS_H

#include "BasinConfigSection.h"
#include "GaugeConfigSection.h"
#include "GaugeMap.h"
#include "Grid.h"
#include "GridNode.h"
#include "Projection.h"
#include <vector>

bool LoadBasicGrids();
void FreeBasicGridsData();
void ClipBasicGrids(long x, long y, long search, const char *output);
void ClipBasicGrids(BasinConfigSection *basin, std::vector<GridNode> *nodes,
                    const char *name, const char *output);
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
    float *defaultInundationParams);
void MakeBasic();
void ReclassifyDDM();
bool CheckESRIDDM();
bool CheckSimpleDDM();

extern FloatGrid *g_DEM;
extern FloatGrid *g_DDM;
extern FloatGrid *g_FAM;
extern Projection *g_Projection;

#endif

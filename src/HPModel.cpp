#include "HPModel.h"
#include "DatedName.h"
#include <cmath>
#include <cstdio>
#include <cstring>

HPModel::HPModel() {}

HPModel::~HPModel() {}

bool HPModel::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (hpNodes.size() != nodes->size()) {
    hpNodes.resize(nodes->size());
  }

  // Fill in modelIndex in the gridNodes
  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    node->modelIndex = i;
  }

  InitializeParameters(paramSettings, paramGrids);

  return true;
}

void HPModel::InitializeStates(TimeVar *beginTime, char *statePath) {}

void HPModel::SaveStates(TimeVar *currentTime, char *statePath,
                         GridWriterFull *gridWriter) {}

bool HPModel::WaterBalance(float stepHours, std::vector<float> *precip,
                           std::vector<float> *pet,
                           std::vector<float> *fastFlow,
                           std::vector<float> *slowFlow,
                           std::vector<float> *soilMoisture) {

  size_t numNodes = nodes->size();

  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    HPGridNode *cNode = &(hpNodes[i]);
    WaterBalanceInt(node, cNode, stepHours, precip->at(i), pet->at(i),
                    &(fastFlow->at(i)), &(slowFlow->at(i)));
    soilMoisture->at(i) = 100.0;
  }

  return true;
}

void HPModel::WaterBalanceInt(GridNode *node, HPGridNode *cNode,
                              float stepHours, float precipIn, float petIn,
                              float *fastFlow, float *slowFlow) {
  float precip = precipIn * stepHours; // precipIn is mm/hr, precip is mm
  float overland_precip = precip * cNode->params[PARAM_HP_SPLIT];
  float interflow_precip = precip * (1.0 - cNode->params[PARAM_HP_SPLIT]);
  // Add Overland Excess Water to fastFlow
  *fastFlow += (overland_precip / (stepHours * 3600.0f));

  // Add Interflow Excess Water to slowFlow
  *slowFlow += (interflow_precip / (stepHours * 3600.0f));
}

void HPModel::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    HPGridNode *cNode = &(hpNodes[i]);
    if (!node->gauge) {
      unused++;
      continue;
    }
    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_HP_QTY);

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_HP_QTY; paramI++) {
      FloatGrid *grid = paramGrids->at(paramI);
      if (grid && g_DEM->IsSpatialMatch(grid)) {
        if (grid->data[node->y][node->x] == 0) {
          grid->data[node->y][node->x] = 0.01;
        }
        cNode->params[paramI] *= grid->data[node->y][node->x];
      } else if (grid &&
                 grid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt)) {
        if (grid->data[pt.y][pt.x] == 0) {
          grid->data[pt.y][pt.x] = 0.01;
          // printf("Using nodata value in param %s\n",
          // modelParamStrings[MODEL_CREST][paramI]);
        }
        cNode->params[paramI] *= grid->data[pt.y][pt.x];
      }
    }

    if (cNode->params[PARAM_HP_PRECIP] < 0.0) {
      printf("Node Precip Multiplier(%f) is less than 0, setting to 0.\n",
             cNode->params[PARAM_HP_PRECIP]);
      cNode->params[PARAM_HP_PRECIP] = 0.0;
    } else if (cNode->params[PARAM_HP_PRECIP] > 1.0) {
      printf("Node Precip Multiplier(%f) is greater than 1, setting to 1.\n",
             cNode->params[PARAM_HP_PRECIP]);
      cNode->params[PARAM_HP_PRECIP] = 1.0;
    }

    if (cNode->params[PARAM_HP_SPLIT] < 0.0) {
      printf("Node Precip Split (%f) is less than 0, setting to 0.\n",
             cNode->params[PARAM_HP_SPLIT]);
      cNode->params[PARAM_HP_SPLIT] = 0.0;
    } else if (cNode->params[PARAM_HP_SPLIT] > 1.0) {
      printf("Node Precip Split (%f) is greater than 1, setting to 1.0.\n",
             cNode->params[PARAM_HP_SPLIT]);
      cNode->params[PARAM_HP_SPLIT] = 1.0;
    }
  }
}

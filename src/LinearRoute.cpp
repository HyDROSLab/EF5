#include "LinearRoute.h"
#include "AscGrid.h"
#include "DatedName.h"
#include <cmath>
#include <cstdio>
#include <cstring>

LRRoute::LRRoute() {}

LRRoute::~LRRoute() {}

float LRRoute::SetObsInflow(long index, float inflow) { return 0.0; }

bool LRRoute::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (lrNodes.size() != nodes->size()) {
    lrNodes.resize(nodes->size());
  }

  // Fill in modelIndex in the gridNodes
  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    node->modelIndex = i;
    LRGridNode *cNode = &(lrNodes[i]);
    cNode->slopeSqrt = pow(node->slope, 0.5f);
  }

  InitializeParameters(paramSettings, paramGrids);
  initialized = false;
  maxSpeed = 1.0;

  return true;
}

void LRRoute::InitializeStates(TimeVar *beginTime, char *statePath,
                               std::vector<float> *fastFlow,
                               std::vector<float> *interFlow,
                               std::vector<float> *baseFlow) {}
void LRRoute::SaveStates(TimeVar *currentTime, char *statePath,
                         GridWriterFull *gridWriter) {}

bool LRRoute::Route(float stepHours, std::vector<float> *fastFlow,
                    std::vector<float> *interFlow,
                    std::vector<float> *baseFlow,
                    std::vector<float> *discharge) {

  if (!initialized) {
    initialized = true;
    InitializeRouting(stepHours * 3600.0f);
  }

  size_t numNodes = nodes->size();

  for (size_t i = 0; i < numNodes; i++) {
    LRGridNode *cNode = &(lrNodes[i]);
    RouteInt(&(nodes->at(i)), cNode, fastFlow->at(i), interFlow->at(i), baseFlow->at(i));
  }

  for (size_t i = 0; i < numNodes; i++) {
    LRGridNode *cNode = &(lrNodes[i]);
    fastFlow->at(i) = cNode->incomingWater[LR_LAYER_OVERLAND];
    cNode->incomingWater[LR_LAYER_OVERLAND] = 0.0;
    interFlow->at(i) = cNode->incomingWater[LR_LAYER_INTERFLOW];
    cNode->incomingWater[LR_LAYER_INTERFLOW] = 0.0;
  }

  InitializeRouting(stepHours * 3600.0f);

  return true;
}

void LRRoute::RouteInt(GridNode *node, LRGridNode *cNode, float fastFlow,
                       float interFlow, float baseFlow) {

  if (!node->channelGridCell) {
    cNode->reservoirs[LR_LAYER_OVERLAND] += fastFlow;
  }

  double overlandLeak =
      cNode->reservoirs[LR_LAYER_OVERLAND] * cNode->params[PARAM_LINEAR_LEAKO];
  cNode->reservoirs[LR_LAYER_OVERLAND] -= overlandLeak;
  if (cNode->reservoirs[LR_LAYER_OVERLAND] < 0) {
    cNode->reservoirs[LR_LAYER_OVERLAND] = 0;
  }

  if (node->channelGridCell) {
    overlandLeak += fastFlow;
  }

  // Add Interflow Excess Water to Reservoir
  cNode->reservoirs[LR_LAYER_INTERFLOW] += interFlow;
  double interflowLeak =
      cNode->reservoirs[LR_LAYER_INTERFLOW] * cNode->params[PARAM_LINEAR_LEAKI];
  cNode->reservoirs[LR_LAYER_INTERFLOW] -= interflowLeak;
  if (cNode->reservoirs[LR_LAYER_INTERFLOW] < 0) {
    cNode->reservoirs[LR_LAYER_INTERFLOW] = 0;
  }

  if (cNode->routeCNode[0][LR_LAYER_OVERLAND]) {
    double overlandLeak0 =
        overlandLeak * cNode->routeAmount[0][LR_LAYER_OVERLAND] * node->area /
        cNode->routeNode[0][LR_LAYER_OVERLAND]->area;
    double leakAmount = overlandLeak0;
    double *res = &(cNode->routeCNode[0][LR_LAYER_OVERLAND]
                        ->incomingWater[LR_LAYER_OVERLAND]);
    *res += leakAmount; // Make this an atomic add for parallelization
  }

  if (cNode->routeCNode[1][LR_LAYER_OVERLAND]) {
    double overlandLeak1 =
        overlandLeak * cNode->routeAmount[1][LR_LAYER_OVERLAND] * node->area /
        cNode->routeNode[1][LR_LAYER_OVERLAND]->area;
    double leakAmount = overlandLeak1;
    double *res = &(cNode->routeCNode[1][LR_LAYER_OVERLAND]
                        ->incomingWater[LR_LAYER_OVERLAND]);
    *res += leakAmount; // Make this an atomic add for parallelization
  }

  if (cNode->routeCNode[0][LR_LAYER_INTERFLOW]) {
    double interflowLeak0 =
        interflowLeak * cNode->routeAmount[0][LR_LAYER_INTERFLOW] * node->area /
        cNode->routeNode[0][LR_LAYER_INTERFLOW]->area;
    double leakAmount = interflowLeak0;
    double *res = &(cNode->routeCNode[0][LR_LAYER_INTERFLOW]
                        ->incomingWater[LR_LAYER_INTERFLOW]);
    *res += leakAmount; // Make this an atomic add for parallelization
  }

  if (cNode->routeCNode[1][LR_LAYER_INTERFLOW]) {
    double interflowLeak1 =
        interflowLeak * cNode->routeAmount[1][LR_LAYER_INTERFLOW] * node->area /
        cNode->routeNode[1][LR_LAYER_INTERFLOW]->area;
    double leakAmount = interflowLeak1;
    double *res = &(cNode->routeCNode[1][LR_LAYER_INTERFLOW]
                        ->incomingWater[LR_LAYER_INTERFLOW]);
    *res += leakAmount; // Make this an atomic add for parallelization
  }
}

void LRRoute::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    LRGridNode *cNode = &(lrNodes[i]);
    if (!node->gauge) {
      unused++;
      continue;
    }
    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_LINEAR_QTY);

    if (!paramGrids->at(PARAM_LINEAR_ISO)) {
      cNode->reservoirs[LR_LAYER_OVERLAND] = cNode->params[PARAM_LINEAR_ISO];
    }
    if (!paramGrids->at(PARAM_LINEAR_ISU)) {
      cNode->reservoirs[LR_LAYER_INTERFLOW] = cNode->params[PARAM_LINEAR_ISU];
    }
    cNode->incomingWater[LR_LAYER_OVERLAND] = 0.0;
    cNode->incomingWater[LR_LAYER_INTERFLOW] = 0.0;

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_LINEAR_QTY; paramI++) {
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

    if (cNode->params[PARAM_LINEAR_LEAKO] < 0.0) {
      printf("Node Leak Overland(%f) is less than 0, setting to 0.\n",
             cNode->params[PARAM_LINEAR_LEAKO]);
      cNode->params[PARAM_LINEAR_LEAKO] = 0.0;
    } else if (cNode->params[PARAM_LINEAR_LEAKO] > 1.0) {
      printf("Node Leak Overland(%f) is greater than 1, setting to 1.\n",
             cNode->params[PARAM_LINEAR_LEAKO]);
      cNode->params[PARAM_LINEAR_LEAKO] = 1.0;
    }

    if (cNode->params[PARAM_LINEAR_LEAKI] < 0.0) {
      printf("Node Leak Interflow(%f) is less than 0, setting to 0.\n",
             cNode->params[PARAM_LINEAR_LEAKI]);
      cNode->params[PARAM_LINEAR_LEAKI] = 0.0;
    } else if (cNode->params[PARAM_LINEAR_LEAKI] > 1.0) {
      printf("Node Leak Interflow(%f) is greater than 1, setting to 1.\n",
             cNode->params[PARAM_LINEAR_LEAKI]);
      cNode->params[PARAM_LINEAR_LEAKI] = 1.0;
    }

    if (node->fac > cNode->params[PARAM_LINEAR_TH]) {
      node->channelGridCell = true;
    } else {
      node->channelGridCell = false;
    }
  }
}

void LRRoute::InitializeRouting(float timeSeconds) {

  maxSpeed = 0;

  // This pass distributes parameters & calculates the time it takes for water
  // to cross the grid cell.
  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    LRGridNode *cNode = &(lrNodes[i]);

    float waterDepth = (cNode->reservoirs[LR_LAYER_OVERLAND] *
                            cNode->params[PARAM_LINEAR_LEAKO] +
                        cNode->reservoirs[LR_LAYER_INTERFLOW] *
                            cNode->params[PARAM_LINEAR_LEAKI]) /
                       1000.0f;
    if (node->channelGridCell) {
      waterDepth += (cNode->incomingWater[LR_LAYER_OVERLAND]) / 1000.0f;
    }
    if (waterDepth < 0.0001) {
      waterDepth = 0.0001;
    }

    // Calculate the approximate speed of the water in meters per second (thus
    // COEM is in meters per second) Slope is meters vertical change over meters
    // horizontal change so thus unitless
    float speed = pow(waterDepth, 0.66) * cNode->slopeSqrt;

    // We have different mannings roughness multipliers for overland & channel
    // grid cells
    if (node->fac > cNode->params[PARAM_LINEAR_TH]) {
      speed *= cNode->params[PARAM_LINEAR_RIVER];
    } else {
      speed *= cNode->params[PARAM_LINEAR_COEM];
    }

    if (speed > maxSpeed) {
      maxSpeed = speed;
    }

    // Calculate the water speed for interflow
    float speedUnder = cNode->params[PARAM_LINEAR_UNDER] * cNode->slopeSqrt;

    float nexTime = node->horLen / speed;
    float nexTimeUnder = node->horLen / speedUnder;
    cNode->nexTime[LR_LAYER_OVERLAND] = nexTime;
    cNode->nexTime[LR_LAYER_INTERFLOW] = nexTimeUnder;
  }

  // This pass figures out which cell water is routed to
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *currentNode, *previousNode;
    float currentSeconds, previousSeconds;
    GridNode *node = &nodes->at(i);
    LRGridNode *cNode = &(lrNodes[i]);

    // Overland routing
    previousSeconds = 0;
    currentSeconds = 0;
    currentNode = node;
    previousNode = NULL;
    while (currentSeconds < timeSeconds) {
      if (currentNode) {
        previousSeconds = currentSeconds;
        previousNode = currentNode;
        currentSeconds +=
            lrNodes[currentNode->modelIndex].nexTime[LR_LAYER_OVERLAND];
        if (currentNode->downStreamNode != INVALID_DOWNSTREAM_NODE) {
          currentNode = &(nodes->at(currentNode->downStreamNode));
        } else {
          currentNode = NULL;
        }
      } else {
        if (timeSeconds > currentSeconds) {
          previousNode = NULL;
        }
        break; // We have effectively run out of nodes to transverse, this is
               // done!
      }
    }

    cNode->routeNode[0][LR_LAYER_OVERLAND] = currentNode;
    cNode->routeCNode[0][LR_LAYER_OVERLAND] =
        (currentNode) ? &(lrNodes[currentNode->modelIndex]) : NULL;
    cNode->routeNode[1][LR_LAYER_OVERLAND] = previousNode;
    cNode->routeCNode[1][LR_LAYER_OVERLAND] =
        (previousNode) ? &(lrNodes[previousNode->modelIndex]) : NULL;
    if ((currentSeconds - previousSeconds) > 0) {
      cNode->routeAmount[0][LR_LAYER_OVERLAND] =
          (timeSeconds - previousSeconds) / (currentSeconds - previousSeconds);
      cNode->routeAmount[1][LR_LAYER_OVERLAND] =
          1.0 - cNode->routeAmount[0][LR_LAYER_OVERLAND];
    }

    // Interflow routing
    previousSeconds = 0;
    currentSeconds = 0;
    currentNode = node;
    previousNode = NULL;
    while (currentSeconds < timeSeconds) {
      if (currentNode) {
        previousSeconds = currentSeconds;
        previousNode = currentNode;
        currentSeconds +=
            lrNodes[currentNode->modelIndex].nexTime[LR_LAYER_INTERFLOW];
        if (currentNode->downStreamNode != INVALID_DOWNSTREAM_NODE) {
          currentNode = &(nodes->at(currentNode->downStreamNode));
        } else {
          currentNode = NULL;
        }
      } else {
        if (timeSeconds > currentSeconds) {
          previousNode = NULL;
        }
        break; // We have effectively run out of nodes to transverse, this is
               // done!
      }
    }

    cNode->routeNode[0][LR_LAYER_INTERFLOW] = currentNode;
    cNode->routeCNode[0][LR_LAYER_INTERFLOW] =
        (currentNode) ? &(lrNodes[currentNode->modelIndex]) : NULL;
    cNode->routeNode[1][LR_LAYER_INTERFLOW] = previousNode;
    cNode->routeCNode[1][LR_LAYER_INTERFLOW] =
        (previousNode) ? &(lrNodes[previousNode->modelIndex]) : NULL;
    if ((currentSeconds - previousSeconds) > 0) {
      cNode->routeAmount[0][LR_LAYER_INTERFLOW] =
          (timeSeconds - previousSeconds) / (currentSeconds - previousSeconds);
      cNode->routeAmount[1][LR_LAYER_INTERFLOW] =
          1.0 - cNode->routeAmount[0][LR_LAYER_INTERFLOW];
    }
  }
}

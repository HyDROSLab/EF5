#include "VCInundation.h"
#include "DatedName.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stack>

static bool TestUpstream(long nextX, long nextY, FLOW_DIR dir, GridLoc *loc);
static bool SortByHeight(GridNode *d1, GridNode *d2);

VCInundation::VCInundation() {}

VCInundation::~VCInundation() {}

bool VCInundation::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (iNodes.size() != nodes->size()) {
    iNodes.resize(nodes->size());
  }

  // Fill in modelIndex in the gridNodes
  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    node->modelIndex = i;
    if (node->channelGridCell) {
      ComputeLayers(i, node, &(iNodes[i]));
    }
  }

  InitializeParameters(paramSettings, paramGrids);

  return true;
}

void VCInundation::ComputeLayers(size_t nodeIndex, GridNode *node,
                                 VCInundationGridNode *cNode) {
  std::vector<GridNode *> upstreamNodes;
  std::stack<GridNode *> walkNodes;

  walkNodes.push(node);

  while (!walkNodes.empty()) {

    // Get the next node to check off the stack
    GridNode *currentN = walkNodes.top();
    walkNodes.pop();

    upstreamNodes.push_back(currentN);

    // Lets figure out what flows into this node
    for (int i = 1; i < FLOW_QTY; i++) {
      GridLoc nextNode;
      if (TestUpstream(currentN->x, currentN->y, (FLOW_DIR)i, &nextNode)) {
        GridNode *nextN = NULL;
        for (size_t nodeI = currentN->index; nodeI < nodes->size(); nodeI++) {
          if (nodes->at(nodeI).x == nextNode.x &&
              nodes->at(nodeI).y == nextNode.y) {
            nextN = &nodes->at(nodeI);
            break;
          }
        }
        if (nextN && !nextN->channelGridCell) {
          walkNodes.push(nextN);
        }
      }
    }
  }

  std::sort(upstreamNodes.begin(), upstreamNodes.end(), SortByHeight);

  int upstreamCount = (int)(upstreamNodes.size()) - 1;
  // printf("Found %i upstream cells for node %i, FAM is %f\n", upstreamCount,
  // (int)nodeIndex, g_FAM->data[node->y][node->x]);
  cNode->layers.reserve(upstreamNodes.size());
  for (int i = 0; i < upstreamCount; i++) {
    GridNode *current = upstreamNodes[i];
    GridNode *up = upstreamNodes[i + 1];
    if (nodeIndex == 54) {
      printf("%i, %f\n", i, g_DEM->data[current->y][current->x]);
    }
    float heightDiff =
        g_DEM->data[up->y][up->x] - g_DEM->data[current->y][current->x];
    if (heightDiff <= 0.01) {
      continue;
      // heightDiff = 0.01;
    }
    float layerVolume = current->area * (i + 1) * heightDiff * 1000000.0;
    VCILayer *layer = new VCILayer;
    layer->totalVolume = layerVolume;
    layer->totalArea = current->area * (i + 1) * 1000000.0;
    layer->height = heightDiff;
    layer->toIndex = i + 1;
    cNode->layers.push_back(layer);
  }

  float layerVolume =
      upstreamNodes[0]->area * (upstreamNodes.size()) * 1000.0 * 1000000.0;
  VCILayer *layer = new VCILayer;
  layer->totalVolume = layerVolume;
  layer->totalArea =
      upstreamNodes[0]->area * (upstreamNodes.size()) * 1000000.0;
  layer->height = 1000.0;
  layer->toIndex = upstreamNodes.size();
  upstreamCount = (int)(upstreamNodes.size());
  cNode->gridIndicies.reserve(upstreamCount);
  for (int gi = 0; gi < upstreamCount; gi++) {
    cNode->gridIndicies.push_back(upstreamNodes[gi]->index);
  }
  cNode->layers.push_back(layer);
}

bool VCInundation::Inundation(std::vector<float> *discharge,
                              std::vector<float> *depth) {

  size_t numNodes = nodes->size();

  for (size_t i = 0; i < numNodes; i++) {
    depth->at(i) = 0.0;
  }

  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    if (node->channelGridCell) {
      VCInundationGridNode *cNode = &(iNodes[i]);
      float dischargeLeft = discharge->at(i) * node->horLen;
      int layerCount = (int)(cNode->layers.size());
      for (int layerI = 0; layerI < layerCount; layerI++) {
        VCILayer *layer = cNode->layers[layerI];
        float volumeUsed = 0.0;
        if (dischargeLeft >= layer->totalVolume) {
          volumeUsed = layer->totalVolume;
          dischargeLeft -= layer->totalVolume;
        } else {
          volumeUsed = dischargeLeft;
          dischargeLeft = 0.0;
        }
        float height = volumeUsed / layer->totalArea;
        int gridCount = (int)(layer->toIndex);
        for (int gi = 0; gi < gridCount; gi++) {
          depth->at(cNode->gridIndicies[gi]) += height;
        }
      }
    }
  }

  return true;
}

void VCInundation::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    VCInundationGridNode *cNode = &(iNodes[i]);
    if (!node->gauge) {
      unused++;
      continue;
    }
    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_VCI_QTY);

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_VCI_QTY; paramI++) {
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
  }
}

bool SortByHeight(GridNode *d1, GridNode *d2) {
  if (g_DEM->data[d1->y][d1->x] == g_DEM->data[d2->y][d2->x]) {
    return d1->index < d2->index;
  }
  return g_DEM->data[d1->y][d1->x] < g_DEM->data[d2->y][d2->x];
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

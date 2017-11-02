#include "SimpleInundation.h"
#include "DatedName.h"
#include <cmath>
#include <cstdio>
#include <cstring>

SimpleInundation::SimpleInundation() {}

SimpleInundation::~SimpleInundation() {}

bool SimpleInundation::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (iNodes.size() != nodes->size()) {
    iNodes.resize(nodes->size());
  }

  InitializeParameters(paramSettings, paramGrids);

  // Fill in modelIndex in the gridNodes
  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    node->modelIndex = i;
  }
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    GridNode *channelNode = node;
    node->modelIndex = i;
    while (!channelNode->channelGridCell &&
           channelNode->downStreamNode != INVALID_DOWNSTREAM_NODE) {
      channelNode = &nodes->at(channelNode->downStreamNode);
    }
    iNodes[i].params[PARAM_SI_ALPHA] =
        iNodes[channelNode->index].params[PARAM_SI_ALPHA];
    iNodes[i].params[PARAM_SI_BETA] =
        iNodes[channelNode->index].params[PARAM_SI_BETA];
    iNodes[i].elevation = g_DEM->data[node->y][node->x];
    iNodes[i].elevationChannel = g_DEM->data[channelNode->y][channelNode->x];
    iNodes[i].elevDiff = iNodes[i].elevation - iNodes[i].elevationChannel;
    iNodes[i].channelIndex = channelNode->index;
  }

  return true;
}

bool SimpleInundation::Inundation(std::vector<float> *discharge,
                                  std::vector<float> *depth) {

  size_t numNodes = nodes->size();

  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    InundationGridNode *cNode = &(iNodes[i]);
    // InundationGridNode *channelNode = &(iNodes[cNode->channelIndex]);
    InundationInt(node, cNode, discharge->at(cNode->channelIndex),
                  &(depth->at(i)));
  }

  return true;
}

void SimpleInundation::InundationInt(GridNode *node, InundationGridNode *cNode,
                                     float dischargeIn, float *depth) {

  // float area =
  // powf(dischargeIn/cNode->params[PARAM_SI_ALPHA], 1.0/cNode->params[PARAM_SI_BETA]);
  // float z1 = 3.0, z2 = 3.0, b = 100.0;
  // float b= 50.0;
  float height = cNode->params[PARAM_SI_ALPHA] *
                 powf(dischargeIn,
                      cNode->params[PARAM_SI_BETA]); // area/b; //(-b +
                                                     // sqrtf(powf(b, 2.0) - 4.0
                                                     // * area * (z1+z2))) / (2.0
                                                     // * (z1+z2));
  // if (node->channelGridCell && dischargeIn != 0.0) {
  //	printf("Have Q %f, area %f, height %f, elev %f\n", dischargeIn, area,
  //height, cNode->elevDiff);
  //}
  float d = height - cNode->elevDiff;
  if (d < 0.0) {
    d = 0.0;
  }
  *depth = d;
}

void SimpleInundation::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    InundationGridNode *cNode = &(iNodes[i]);
    if (!node->gauge) {
      unused++;
      continue;
    }
    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_SI_QTY);

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_SI_QTY; paramI++) {
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

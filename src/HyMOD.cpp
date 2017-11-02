#include "HyMOD.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

HyMOD::HyMOD() {}

HyMOD::~HyMOD() {}

bool HyMOD::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (hymodNodes.size() != nodes->size()) {
    hymodNodes.resize(nodes->size());
  }

  // Fill in modelNode in the gridNodes
  std::vector<HyMODGridNode>::iterator citr = hymodNodes.begin();
  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    HyMODGridNode *cNode = &(*citr);

    node->modelNode = cNode;

    citr++;
  }

  InitializeParameters(paramSettings);

  return true;
}

void HyMOD::InitializeStates(TimeVar *beginTime, char *statePath) {}

void HyMOD::SaveStates(TimeVar *currentTime, char *statePath,
                       GridWriterFull *gridWriter) {}

bool HyMOD::WaterBalance(float stepHours, std::vector<float> *precip,
                         std::vector<float> *pet, std::vector<float> *fastFlow,
                         std::vector<float> *slowFlow,
                         std::vector<float> *soilMoisture) {

  size_t numNodes = nodes->size();
  size_t i = 0;

  for (i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    HyMODGridNode *cNode = (HyMODGridNode *)node->modelNode;
    WaterBalanceInt(node, cNode, stepHours, precip->at(i), pet->at(i));
    LocalRouteQF(node, cNode, stepHours);
    LocalRouteSF(node, cNode, stepHours);
    fastFlow->at(i) = cNode->dischargeQF;
    slowFlow->at(i) = cNode->dischargeSF;
    soilMoisture->at(i) = 0;
    // discharge->at(i) = (cNode->dischargeQF + cNode->dischargeSF) * node->area
    // * 0.277777777777778f / stepHours; // Convert from mm/time to cms
  }

  return true;
}

void HyMOD::WaterBalanceInt(GridNode *node, HyMODGridNode *cNode,
                            float stepHours, float precipIn, float petIn) {

  float precip =
      precipIn * stepHours *
      cNode->params[PARAM_HYMOD_PRECIP]; // precipIn is mm/hr, precip is mm
  float pet = petIn * stepHours;         // petIn in mm/hr, pet is mm

  float Cbeg =
      cNode->CPar * (1 - pow(1 - (cNode->XHuz / cNode->params[PARAM_HYMOD_HUZ]),
                             1 + cNode->b)); // Contents at begining
  float OV2 = std::max(
      0.0f, precip + cNode->XHuz -
                cNode->params[PARAM_HYMOD_HUZ]); // Compute OV2 if enough PP
  float PPinf = precip - OV2;                    // PP that does not go to OV2
  float Hint = std::min(cNode->params[PARAM_HYMOD_HUZ],
                        PPinf + cNode->XHuz); // Intermediate height
  float Cint = cNode->CPar * (1 - pow(1 - Hint / cNode->params[PARAM_HYMOD_HUZ],
                                      1 + cNode->b)); // Intermediate contents
  float OV1 = std::max(0.0f, PPinf + Cbeg - Cint);    // Compute OV1
  float OV = OV1 + OV2;                               // Compute total OV
  float ET = std::min(pet, Cint);                     // Compute ET
  float Cend = Cint - ET;                             // Final contents
  float Hend =
      cNode->params[PARAM_HYMOD_HUZ] *
      (1 -
       pow(1 - Cend / cNode->CPar,
           1 / (1 + cNode->b))); // Final height corresponding to SMA contents

  // Update states
  cNode->XHuz = Hend;
  cNode->XCuz = Cend;

  cNode->precipExcess = OV;
}

void HyMOD::LocalRouteQF(GridNode *node, HyMODGridNode *cNode,
                         float stepHours) {
  float inFlow = cNode->params[PARAM_HYMOD_ALP] * cNode->precipExcess;

  float prevTransfer = inFlow;

  for (int i = 0; i < cNode->numQF; i++) {
    float transfer = cNode->params[PARAM_HYMOD_KQ] * stepHours * cNode->Xq[i];
    cNode->Xq[i] = cNode->Xq[i] - transfer + prevTransfer;
    prevTransfer = transfer;
  }

  cNode->dischargeQF = prevTransfer;
}

void HyMOD::LocalRouteSF(GridNode *node, HyMODGridNode *cNode,
                         float stepHours) {
  float inFlow = (1 - cNode->params[PARAM_HYMOD_ALP]) * cNode->precipExcess;

  float transfer = cNode->params[PARAM_HYMOD_KS] * stepHours * cNode->Xs;
  cNode->Xs = cNode->Xs - transfer + inFlow;

  cNode->dischargeSF = transfer;
}

void HyMOD::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings) {

  // This pass distributes parameters
  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    HyMODGridNode *cNode = (HyMODGridNode *)node->modelNode;

    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_HYMOD_QTY);

    cNode->params[PARAM_HYMOD_KS] /= 24.0f;
    cNode->params[PARAM_HYMOD_KQ] /= 24.0f;

    // Some of the parameters are special, deal with that here
    cNode->b = log(1.0 - cNode->params[PARAM_HYMOD_B] / 2.0) / log(0.5);
    cNode->CPar = cNode->params[PARAM_HYMOD_HUZ] / (1 + cNode->b);
    cNode->numQF = (int)cNode->params[PARAM_HYMOD_NQ];

    // States
    cNode->XCuz = cNode->params[PARAM_HYMOD_XCUZ] * cNode->CPar;
    cNode->XHuz =
        cNode->params[PARAM_HYMOD_HUZ] *
        (1 - pow((1 - cNode->XCuz / cNode->CPar), 1 / (1 + cNode->b)));
    if (cNode->XHuz != cNode->XHuz) {
      cNode->XHuz = 0;
    }
    cNode->Xs = cNode->params[PARAM_HYMOD_XS];
    if (cNode->Xq) {
      delete[] cNode->Xq;
    }
    cNode->Xq = new float[cNode->numQF];
    for (int i = 0; i < cNode->numQF; i++) {
      cNode->Xq[i] = cNode->params[PARAM_HYMOD_XQ];
    }
  }
}

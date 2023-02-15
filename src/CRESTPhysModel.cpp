#include <cmath>
#include <cstdio>
#include <cstring>
#if _OPENMP
#include <omp.h>
#endif
#include "AscGrid.h"
#include "CRESTPhysModel.h"
#include "DatedName.h"

static const char *stateStrings[] = {
    "SM",
    "GW"
};

CRESTPHYSModel::CRESTPHYSModel() {}

CRESTPHYSModel::~CRESTPHYSModel() {}

bool CRESTPHYSModel::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (crestphysNodes.size() != nodes->size()) {
    crestphysNodes.resize(nodes->size());
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

void CRESTPHYSModel::InitializeStates(TimeVar *beginTime, char *statePath) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(beginTime->GetTM());

  char buffer[300];
  for (int p = 0; p < STATE_CRESTPHYS_QTY; p++) {
    sprintf(buffer, "%s/crestphys_%s_%s.tif", statePath, stateStrings[p],
            timeStr.GetName());

    FloatGrid *sGrid = ReadFloatTifGrid(buffer);
    if (sGrid) {
      printf("Using CRESTPHYS %s State Grid %s\n", stateStrings[p], buffer);
      if (g_DEM->IsSpatialMatch(sGrid)) {
        for (size_t i = 0; i < nodes->size(); i++) {
          GridNode *node = &nodes->at(i);
          CRESTPHYSGridNode *cNode = &(crestphysNodes[i]);
          if (sGrid->data[node->y][node->x] != sGrid->noData) {
            cNode->states[p] = sGrid->data[node->y][node->x];
          }
        }
      } else {
        GridLoc pt;
        for (size_t i = 0; i < nodes->size(); i++) {
          GridNode *node = &(nodes->at(i));
          CRESTPHYSGridNode *cNode = &(crestphysNodes[i]);
          if (sGrid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) &&
              sGrid->data[pt.y][pt.x] != sGrid->noData) {
            cNode->states[p] = sGrid->data[pt.y][pt.x];
          }
        }
      }
      delete sGrid;
    } else {
      printf("CRESTPHYS %s State Grid %s not found!\n", stateStrings[p], buffer);
    }
  }
}

void CRESTPHYSModel::SaveStates(TimeVar *currentTime, char *statePath,
                            GridWriterFull *gridWriter) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(currentTime->GetTM());

  std::vector<float> dataVals;
  dataVals.resize(nodes->size());

  char buffer[300];
  for (int p = 0; p < STATE_CRESTPHYS_QTY; p++) {
    sprintf(buffer, "%s/crestphys_%s_%s.tif", statePath, stateStrings[p],
            timeStr.GetName());
    for (size_t i = 0; i < nodes->size(); i++) {
      CRESTPHYSGridNode *cNode = &(crestphysNodes[i]);
      dataVals[i] = cNode->states[p];
    }
    gridWriter->WriteGrid(nodes, &dataVals, buffer, false);
  }
}

bool CRESTPHYSModel::WaterBalance(float stepHours,
                              std::vector<float> *precip,
                              std::vector<float> *pet,
                              std::vector<float> *fastFlow,
                              std::vector<float> *interFlow,
                              std::vector<float> *baseFlow,
                              std::vector<float> *soilMoisture,
                              std::vector<float> *groundwater) {

  size_t numNodes = nodes->size();

#if _OPENMP
  //#pragma omp parallel for
#endif
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    CRESTPHYSGridNode *cNode = &(crestphysNodes[i]);
    WaterBalanceInt(node,
                    cNode,
                    stepHours,
                    precip->at(i),
                    pet->at(i),
                    &(fastFlow->at(i)),
                    &(interFlow->at(i)),
                    &(baseFlow->at(i)));
    soilMoisture->at(i) =
        cNode->states[STATE_CRESTPHYS_SM] * 100.0 / cNode->params[PARAM_CREST_WM];
    groundwater ->at(i) = cNode->states[STATE_CRESTPHYS_GW];
  }
    

  return true;
}

void CRESTPHYSModel::WaterBalanceInt(GridNode *node, CRESTPHYSGridNode *cNode,
                                 float stepHours, float precipIn, float petIn,
                                 float *fastFlow, float *interFlow, float *baseFlow) {
  double precip = precipIn * stepHours; // precipIn is mm/hr, precip is mm
  double pet = petIn * stepHours;       // petIn in mm/hr, pet is mm
  double R = 0.0, Wo = 0.0;
  float baseflowExcess=0.0;

  double adjPET = pet * cNode->params[PARAM_CRESTPHYS_KE];
  double temX = 0.0;

  // If we aren't a channel cell, add routed in overland to precip
  /*if (!node->channelGridCell) {
    precip += *fastFlow;
    *fastFlow = 0.0;
  }*/

  // Base flow continuity
  cNode->states[STATE_CRESTPHYS_GW]+= *baseFlow;

  // We have more water coming in than leaving via ET.
  if (precip > adjPET) {
    double precipSoil =
        (precip - adjPET) * (1 - cNode->params[PARAM_CRESTPHYS_IM]); // This is the
                                                                 // precip that
                                                                 // makes it to
                                                                 // the soil
    double precipImperv =
        precip - adjPET - precipSoil; // Portion of precip on impervious area

    // cNode->states[STATE_CRESTPHYS_SM] += *slowFlow;
    //*slowFlow = 0.0;

    double interflowExcess =
        cNode->states[STATE_CRESTPHYS_SM] - cNode->params[PARAM_CRESTPHYS_WM];
    if (interflowExcess < 0.0) {
      interflowExcess = 0.0;
    }

    if (cNode->states[STATE_CRESTPHYS_SM] > cNode->params[PARAM_CRESTPHYS_WM]) {
      cNode->states[STATE_CRESTPHYS_SM] = cNode->params[PARAM_CRESTPHYS_WM];
    }

    if (cNode->states[STATE_CRESTPHYS_SM] < cNode->params[PARAM_CRESTPHYS_WM]) {
      double Wmaxm =
          cNode->params[PARAM_CRESTPHYS_WM] * (1 + cNode->params[PARAM_CRESTPHYS_B]);
      double A = Wmaxm * (1 - pow(1 - cNode->states[STATE_CRESTPHYS_SM] /
                                          cNode->params[PARAM_CRESTPHYS_WM],
                                  1 / (1 + cNode->params[PARAM_CRESTPHYS_B])));
      if (precipSoil + A >= Wmaxm) {
        R = precipSoil -
            (cNode->params[PARAM_CRESTPHYS_WM] -
             cNode->states[STATE_CRESTPHYS_SM]); // Leftovers after filling SM

        if (R < 0) {
          printf("R to %f, [%f, %f, %f, %f, %f, %f]\n", R,
                 cNode->params[PARAM_CRESTPHYS_WM], cNode->params[PARAM_CRESTPHYS_B],
                 cNode->states[STATE_CRESTPHYS_SM], A, Wmaxm, precipSoil);
          R = 0.0;
        }

        Wo = cNode->params[PARAM_CRESTPHYS_WM];

      } else {
        double infiltration =
            cNode->params[PARAM_CRESTPHYS_WM] *
            (pow(1 - A / Wmaxm, 1 + cNode->params[PARAM_CRESTPHYS_B]) -
             pow(1 - (A + precipSoil) / Wmaxm,
                 1 + cNode->params[PARAM_CRESTPHYS_B]));
        if (infiltration > precipSoil) {
          infiltration = precipSoil;
        } else if (infiltration < 0.0) {
          printf("Infiltration went to %f, [%f, %f, %f, %f, %f, %f]\n",
                 infiltration, cNode->params[PARAM_CRESTPHYS_WM],
                 cNode->params[PARAM_CRESTPHYS_B], cNode->states[STATE_CRESTPHYS_SM], A,
                 Wmaxm, precipSoil);
        }

        R = precipSoil - infiltration;

        if (R < 0) {
          printf("R (infil) to %f, [%f, %f, %f, %f, %f, %f]\n", R,
                 cNode->params[PARAM_CRESTPHYS_WM], cNode->params[PARAM_CRESTPHYS_B],
                 cNode->states[STATE_CRESTPHYS_SM], A, Wmaxm, precipSoil);
          R = 0.0;
        }
        Wo = cNode->states[STATE_CRESTPHYS_SM] + infiltration;
      }
    } else {
      R = precipSoil;
      Wo = cNode->params[PARAM_CRESTPHYS_WM];
    }

    // Now R is excess water, split it between overland & interflow

    temX = (cNode->states[STATE_CRESTPHYS_SM] + Wo) /
           cNode->params[PARAM_CRESTPHYS_WM] / 2 *
           (cNode->params[PARAM_CRESTPHYS_FC] *
            stepHours); // Calculate how much water can infiltrate

    if (R <= temX) {
      cNode->excess[CRESTPHYS_LAYER_BASEFLOW] = R;
    } else {
      cNode->excess[CRESTPHYS_LAYER_BASEFLOW] = temX;
    }
    cNode->excess[CRESTPHYS_LAYER_INTERFLOW] =
        R - cNode->excess[CRESTPHYS_LAYER_BASEFLOW];
    
    cNode->excess[CRESTPHYS_LAYER_OVERLAND]= precipImperv;

    cNode->actET = adjPET;

    cNode->excess[CRESTPHYS_LAYER_OVERLAND] +=
        interflowExcess; // Extra interflow that got routed.
  } else {               // All the incoming precip goes straight to ET
    cNode->excess[CRESTPHYS_LAYER_OVERLAND] = 0.0;
    cNode->excess[CRESTPHYS_LAYER_BASEFLOW] = 0.0;
    cNode->excess[CRESTPHYS_LAYER_INTERFLOW] = 0.0;

    // cNode->states[STATE_CRESTPHYS_SM] += *slowFlow;
    //*slowFlow = 0.0;

    double interflowExcess =
        cNode->states[STATE_CRESTPHYS_SM] - cNode->params[PARAM_CRESTPHYS_WM];
    if (interflowExcess < 0.0) {
      interflowExcess = 0.0;
    }
    cNode->excess[CRESTPHYS_LAYER_OVERLAND] = interflowExcess;

    if (cNode->states[STATE_CRESTPHYS_SM] > cNode->params[PARAM_CRESTPHYS_WM]) {
      cNode->states[STATE_CRESTPHYS_SM] = cNode->params[PARAM_CRESTPHYS_WM];
    }

    double ExcessET = (adjPET - precip) * cNode->states[STATE_CRESTPHYS_SM] /
                      cNode->params[PARAM_CRESTPHYS_WM];
    if (ExcessET < cNode->states[STATE_CRESTPHYS_SM]) {
      Wo = cNode->states[STATE_CRESTPHYS_SM] -
           ExcessET; // We can evaporate away ExcessET too.
    } else {
      Wo = 0.0; // We don't have enough to evaporate ExcessET.
      ExcessET = cNode->states[STATE_CRESTPHYS_SM];
    }
    double resET = (adjPET - ExcessET - precip)*cNode->states[STATE_CRESTPHYS_GW]/cNode->params[PARAM_CRESTPHYS_HMAXAQ];
    
    if (resET<cNode->states[STATE_CRESTPHYS_GW]){
      cNode->states[STATE_CRESTPHYS_GW]-= resET;
    }
    else {
      resET= cNode->states[STATE_CRESTPHYS_GW];
      cNode->states[STATE_CRESTPHYS_GW]-= resET;
    }
    // printf("groundwater 0: %f\n", cNode->states[STATE_CRESTPHYS_GW]);
    
    // printf("groundwater 1: %f\n", cNode->states[STATE_CRESTPHYS_GW]);
    cNode->actET = resET+ExcessET + precip;
  }

  // Modified by Allen 02/14/23, we drain soil moisture based on a rate factor KSOIL
  double soil2gw=Wo*cNode->params[PARAM_CRESTPHYS_KSOIL];
  if (Wo>=soil2gw){
    Wo-= soil2gw;
  }
  else{
    Wo = 0.0;
    soil2gw=Wo;
  }

  cNode->states[STATE_CRESTPHYS_SM] = Wo;
  cNode->states[STATE_CRESTPHYS_GW] += (cNode->excess[CRESTPHYS_LAYER_BASEFLOW]+soil2gw);
  // printf("groundwater 2: %f\n", cNode->states[STATE_CRESTPHYS_GW]);
  // Add Overland Excess Water to fastFlow
  *fastFlow += (cNode->excess[CRESTPHYS_LAYER_OVERLAND] / (stepHours * 3600.0f));

  // Add Interflow Excess Water to slowFlow
  *interFlow += (cNode->excess[CRESTPHYS_LAYER_INTERFLOW] / (stepHours * 3600.0f));

  // Base flow we apply a fill-spill bucket
  if (cNode->states[STATE_CRESTPHYS_GW]>cNode->params[PARAM_CRESTPHYS_HMAXAQ]) {
    // spill
    baseflowExcess= cNode->states[STATE_CRESTPHYS_GW]-cNode->params[PARAM_CRESTPHYS_HMAXAQ];
    cNode->states[STATE_CRESTPHYS_GW]=cNode->params[PARAM_CRESTPHYS_HMAXAQ];
    // printf("set groundwater level %.2f to maximum aquifer depth %.2f...\n",cNode->states[STATE_CRESTPHYS_GW], cNode->params[PARAM_CRESTPHYS_HMAXAQ]);
  }
  else{
    baseflowExcess=0.0;
  }
  double baseflowExp= cNode->params[PARAM_CRESTPHYS_GWC]*(exp(cNode->params[PARAM_CRESTPHYS_GWE]*
                        cNode->states[STATE_CRESTPHYS_GW]/cNode->params[PARAM_CRESTPHYS_HMAXAQ])-1);
  // printf("GWC: %f, GWE: %f, GW: %f, HMAX: %f, baseflow: %f\n", cNode->params[PARAM_CRESTPHYS_GWC],
  //   cNode->params[PARAM_CRESTPHYS_GWE], cNode->states[STATE_CRESTPHYS_GW],cNode->params[PARAM_CRESTPHYS_HMAXAQ], baseflowExp);
  // printf("groundwater 3: %f\n", cNode->states[STATE_CRESTPHYS_GW]);                      
  if (baseflowExp>cNode->states[STATE_CRESTPHYS_GW]){
    baseflowExp= cNode->states[STATE_CRESTPHYS_GW];
    cNode->states[STATE_CRESTPHYS_GW]=0;
  }
  else{
    cNode->states[STATE_CRESTPHYS_GW]-=baseflowExp;
  }
  // printf("groundwater flow : %f\n", baseflowExp);
  // printf("groundwater 4: %f\n", cNode->states[STATE_CRESTPHYS_GW]);
  *baseFlow = ((baseflowExcess+baseflowExp) / (stepHours * 3600.0f));
  // printf("baseflow: %f\n", *baseFlow);

  // Calculate Discharge as the sum of the leaks
  //*discharge = (overlandLeak + interflowLeak) * node->area / 3.6;
}

void CRESTPHYSModel::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    CRESTPHYSGridNode *cNode = &(crestphysNodes[i]);
    if (!node->gauge) {
      unused++;
      continue;
    }
    /*if (i == 0) {
            float *paramsBlah = (*paramSettings)[node->gauge];
            printf("Thread %i, %f\n", omp_get_thread_num(),
    paramsBlah[PARAM_CREST_QTY-1]);
    }*/
    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_CRESTPHYS_QTY);

    // Some of the parameters are special, deal with that here
    if (!paramGrids->at(PARAM_CRESTPHYS_IM)) {
      cNode->params[PARAM_CRESTPHYS_IM] /= 100.0;
    }

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_CRESTPHYS_QTY; paramI++) {
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

    if (!paramGrids->at(PARAM_CRESTPHYS_IWU)) {
      cNode->states[STATE_CRESTPHYS_SM] = cNode->params[PARAM_CRESTPHYS_IWU] *
                                      cNode->params[PARAM_CRESTPHYS_WM] / 100.0;
    }

    if (!paramGrids->at(PARAM_CRESTPHYS_IGW)) {
      cNode->states[STATE_CRESTPHYS_GW] = cNode->params[PARAM_CRESTPHYS_IGW] *
                                      cNode->params[PARAM_CRESTPHYS_HMAXAQ] / 100.0;
    }    

    if (cNode->params[PARAM_CRESTPHYS_WM] < 0.0) {
      cNode->params[PARAM_CRESTPHYS_WM] = 100.0;
    }
    // printf("Maximum aquifer depth: %f\n",cNode->params[PARAM_CRESTPHYS_HMAXAQ]);
    if (cNode->states[STATE_CRESTPHYS_SM] < 0.0) {
      printf("Node Soil Moisture(%f) is less than 0, setting to 0.\n",
             cNode->states[STATE_CRESTPHYS_SM]);
      cNode->states[STATE_CRESTPHYS_SM] = 0.0;
    } else if (cNode->states[STATE_CRESTPHYS_SM] > cNode->params[PARAM_CREST_WM]) {
      printf("Node Soil Moisture(%f) is greater than WM, setting to %f.\n",
             cNode->states[STATE_CRESTPHYS_SM], cNode->params[PARAM_CREST_WM]);
    }

    if (cNode->params[PARAM_CRESTPHYS_IM] < 0.0) {
      // printf("Node Impervious Area(%f) is less than 0, setting to 0.\n",
      // cNode->params[PARAM_CREST_IM]);
      cNode->params[PARAM_CRESTPHYS_IM] = 0.0;
    } else if (cNode->params[PARAM_CRESTPHYS_IM] > 1.0) {
      // printf("Node Impervious Area(%f) is greater than 1, setting to 1.\n",
      // cNode->params[PARAM_CREST_IM]);
      cNode->params[PARAM_CRESTPHYS_IM] = 1.0;
    }

    if (cNode->params[PARAM_CRESTPHYS_B] < 0.0) {
      // printf("Node B (%f) is less than 0, setting to 0.\n",
      // cNode->params[PARAM_CREST_B]);
      cNode->params[PARAM_CRESTPHYS_B] = 1.0;
    } else if (cNode->params[PARAM_CRESTPHYS_B] != cNode->params[PARAM_CRESTPHYS_B]) {
      // printf("Node B (%f) NaN, setting to %f.\n",
      // cNode->params[PARAM_CREST_B], 0.0);
      cNode->params[PARAM_CRESTPHYS_B] = 0.0;
    }

    if (cNode->params[PARAM_CRESTPHYS_FC] < 0.0) {
      // printf("Node B (%f) is less than 0, setting to 0.\n",
      // cNode->params[PARAM_CREST_B]);
      cNode->params[PARAM_CRESTPHYS_FC] = 1.0;
    }
    if (cNode->params[PARAM_CRESTPHYS_IGW]<0.0){
      cNode->params[PARAM_CRESTPHYS_IGW]=0.0;
    }

    if (cNode->params[PARAM_CRESTPHYS_HMAXAQ]<0.0){
      cNode->params[PARAM_CRESTPHYS_HMAXAQ]=0.1;
    }

    if (cNode->params[PARAM_CRESTPHYS_GWE]<0.0){
      cNode->params[PARAM_CRESTPHYS_GWE]=0;
    }

    if (cNode->params[PARAM_CRESTPHYS_GWC]<0.0){
      cNode->params[PARAM_CRESTPHYS_GWC]=0;
    }

  }
}

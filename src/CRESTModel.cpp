#include <cmath>
#include <cstdio>
#include <cstring>
#if _OPENMP
#include <omp.h>
#endif
#include "AscGrid.h"
#include "CRESTModel.h"
#include "DatedName.h"

static const char *stateStrings[] = {
    "SM",
    "GW",
};

CRESTModel::CRESTModel() {}

CRESTModel::~CRESTModel() {}

bool CRESTModel::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (crestNodes.size() != nodes->size()) {
    crestNodes.resize(nodes->size());
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

void CRESTModel::InitializeStates(TimeVar *beginTime, char *statePath) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(beginTime->GetTM());

  char buffer[300];
  for (int p = 0; p < STATE_CREST_QTY; p++) {
    sprintf(buffer, "%s/crest_%s_%s.tif", statePath, stateStrings[p],
            timeStr.GetName());

    FloatGrid *sGrid = ReadFloatTifGrid(buffer);
    if (sGrid) {
      printf("Using CREST %s State Grid %s\n", stateStrings[p], buffer);
      if (g_DEM->IsSpatialMatch(sGrid)) {
        for (size_t i = 0; i < nodes->size(); i++) {
          GridNode *node = &nodes->at(i);
          CRESTGridNode *cNode = &(crestNodes[i]);
          if (sGrid->data[node->y][node->x] != sGrid->noData) {
            cNode->states[p] = sGrid->data[node->y][node->x];
          }
        }
      } else {
        GridLoc pt;
        for (size_t i = 0; i < nodes->size(); i++) {
          GridNode *node = &(nodes->at(i));
          CRESTGridNode *cNode = &(crestNodes[i]);
          if (sGrid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) &&
              sGrid->data[pt.y][pt.x] != sGrid->noData) {
            cNode->states[p] = sGrid->data[pt.y][pt.x];
          }
        }
      }
      delete sGrid;
    } else {
      printf("CREST %s State Grid %s not found!\n", stateStrings[p], buffer);
    }
  }
}

void CRESTModel::SaveStates(TimeVar *currentTime, char *statePath,
                            GridWriterFull *gridWriter) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(currentTime->GetTM());

  std::vector<float> dataVals;
  dataVals.resize(nodes->size());

  char buffer[300];
  for (int p = 0; p < STATE_CREST_QTY; p++) {
    sprintf(buffer, "%s/crest_%s_%s.tif", statePath, stateStrings[p],
            timeStr.GetName());
    for (size_t i = 0; i < nodes->size(); i++) {
      CRESTGridNode *cNode = &(crestNodes[i]);
      dataVals[i] = cNode->states[p];
    }
    gridWriter->WriteGrid(nodes, &dataVals, buffer, false);
  }
}

bool CRESTModel::WaterBalance(float stepHours,
                              std::vector<float> *precip,
                              std::vector<float> *pet,
                              std::vector<float> *fastFlow,
                              std::vector<float> *slowFlow,
                              std::vector<float> *baseFlow,
                              std::vector<float> *soilMoisture,
                              std::vector<float> *groundwater) {

  size_t numNodes = nodes->size();

#if _OPENMP
  //#pragma omp parallel for
#endif

  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    CRESTGridNode *cNode = &(crestNodes[i]);
    WaterBalanceInt(node, cNode, stepHours, precip->at(i), pet->at(i),
                    &(fastFlow->at(i)), &(slowFlow->at(i)),
                    &(baseFlow->at(i)));
    soilMoisture->at(i) =
        cNode->states[STATE_CREST_SM] * 100.0 / cNode->params[PARAM_CREST_WM];
  }

  return true;
}

void CRESTModel::WaterBalanceInt(GridNode *node, CRESTGridNode *cNode,
                                 float stepHours, float precipIn, float petIn,
                                 float *fastFlow, float *slowFlow, float *baseFlow) {
  double precip = precipIn * stepHours; // precipIn is mm/hr, precip is mm
  double pet = petIn * stepHours;       // petIn in mm/hr, pet is mm
  double R = 0.0, Wo = 0.0;

  double adjPET = pet * cNode->params[PARAM_CREST_KE];
  double temX = 0.0;

  // If we aren't a channel cell, add routed in overland to precip
  /*if (!node->channelGridCell) {
    precip += *fastFlow;
    *fastFlow = 0.0;
  }*/

  // We have more water coming in than leaving via ET.
  if (precip > adjPET) {
    double precipSoil =
        (precip - adjPET) * (1 - cNode->params[PARAM_CREST_IM]); // This is the
                                                                 // precip that
                                                                 // makes it to
                                                                 // the soil
    double precipImperv =
        precip - adjPET - precipSoil; // Portion of precip on impervious area

    // cNode->states[STATE_CREST_SM] += *slowFlow;
    //*slowFlow = 0.0;

    double interflowExcess =
        cNode->states[STATE_CREST_SM] - cNode->params[PARAM_CREST_WM];
    if (interflowExcess < 0.0) {
      interflowExcess = 0.0;
    }

    if (cNode->states[STATE_CREST_SM] > cNode->params[PARAM_CREST_WM]) {
      cNode->states[STATE_CREST_SM] = cNode->params[PARAM_CREST_WM];
    }

    if (cNode->states[STATE_CREST_SM] < cNode->params[PARAM_CREST_WM]) {
      double Wmaxm =
          cNode->params[PARAM_CREST_WM] * (1 + cNode->params[PARAM_CREST_B]);
      double A = Wmaxm * (1 - pow(1 - cNode->states[STATE_CREST_SM] /
                                          cNode->params[PARAM_CREST_WM],
                                  1 / (1 + cNode->params[PARAM_CREST_B])));
      if (precipSoil + A >= Wmaxm) {
        R = precipSoil -
            (cNode->params[PARAM_CREST_WM] -
             cNode->states[STATE_CREST_SM]); // Leftovers after filling SM

        if (R < 0) {
          printf("R to %f, [%f, %f, %f, %f, %f, %f]\n", R,
                 cNode->params[PARAM_CREST_WM], cNode->params[PARAM_CREST_B],
                 cNode->states[STATE_CREST_SM], A, Wmaxm, precipSoil);
          R = 0.0;
        }

        Wo = cNode->params[PARAM_CREST_WM];

      } else {
        double infiltration =
            cNode->params[PARAM_CREST_WM] *
            (pow(1 - A / Wmaxm, 1 + cNode->params[PARAM_CREST_B]) -
             pow(1 - (A + precipSoil) / Wmaxm,
                 1 + cNode->params[PARAM_CREST_B]));
        if (infiltration > precipSoil) {
          infiltration = precipSoil;
        } else if (infiltration < 0.0) {
          printf("Infiltration went to %f, [%f, %f, %f, %f, %f, %f]\n",
                 infiltration, cNode->params[PARAM_CREST_WM],
                 cNode->params[PARAM_CREST_B], cNode->states[STATE_CREST_SM], A,
                 Wmaxm, precipSoil);
        }

        R = precipSoil - infiltration;

        if (R < 0) {
          printf("R (infil) to %f, [%f, %f, %f, %f, %f, %f]\n", R,
                 cNode->params[PARAM_CREST_WM], cNode->params[PARAM_CREST_B],
                 cNode->states[STATE_CREST_SM], A, Wmaxm, precipSoil);
          R = 0.0;
        }
        Wo = cNode->states[STATE_CREST_SM] + infiltration;
      }
    } else {
      R = precipSoil;
      Wo = cNode->params[PARAM_CREST_WM];
    }

    // Now R is excess water, split it between overland & interflow

    temX = (cNode->states[STATE_CREST_SM] + Wo) /
           cNode->params[PARAM_CREST_WM] / 2 *
           (cNode->params[PARAM_CREST_FC] *
            stepHours); // Calculate how much water can infiltrate

    if (R <= temX) {
      cNode->excess[CREST_LAYER_INTERFLOW] = R;
    } else {
      cNode->excess[CREST_LAYER_INTERFLOW] = temX;
    }
    cNode->excess[CREST_LAYER_OVERLAND] =
        R - cNode->excess[CREST_LAYER_INTERFLOW] + precipImperv;

    cNode->actET = adjPET;

    cNode->excess[CREST_LAYER_INTERFLOW] +=
        interflowExcess; // Extra interflow that got routed.
  } else {               // All the incoming precip goes straight to ET
    cNode->excess[CREST_LAYER_OVERLAND] = 0.0;

    // cNode->states[STATE_CREST_SM] += *slowFlow;
    //*slowFlow = 0.0;

    double interflowExcess =
        cNode->states[STATE_CREST_SM] - cNode->params[PARAM_CREST_WM];
    if (interflowExcess < 0.0) {
      interflowExcess = 0.0;
    }
    cNode->excess[CREST_LAYER_INTERFLOW] = interflowExcess;

    if (cNode->states[STATE_CREST_SM] > cNode->params[PARAM_CREST_WM]) {
      cNode->states[STATE_CREST_SM] = cNode->params[PARAM_CREST_WM];
    }

    double ExcessET = (adjPET - precip) * cNode->states[STATE_CREST_SM] /
                      cNode->params[PARAM_CREST_WM];
    if (ExcessET < cNode->states[STATE_CREST_SM]) {
      Wo = cNode->states[STATE_CREST_SM] -
           ExcessET; // We can evaporate away ExcessET too.
    } else {
      Wo = 0.0; // We don't have enough to evaporate ExcessET.
      ExcessET = cNode->states[STATE_CREST_SM];
    }
    cNode->actET = ExcessET + precip;
  }

  cNode->states[STATE_CREST_SM] = Wo;

  // Add Overland Excess Water to fastFlow
  *fastFlow += (cNode->excess[CREST_LAYER_OVERLAND] / (stepHours * 3600.0f));

  // Add Interflow Excess Water to slowFlow
  *slowFlow += (cNode->excess[CREST_LAYER_INTERFLOW] / (stepHours * 3600.0f));

  // Calculate Discharge as the sum of the leaks
  //*discharge = (overlandLeak + interflowLeak) * node->area / 3.6;
}

void CRESTModel::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    CRESTGridNode *cNode = &(crestNodes[i]);
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
           sizeof(float) * PARAM_CREST_QTY);

    // Some of the parameters are special, deal with that here
    if (!paramGrids->at(PARAM_CREST_IM)) {
      cNode->params[PARAM_CREST_IM] /= 100.0;
    }

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_CREST_QTY; paramI++) {
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

    if (!paramGrids->at(PARAM_CREST_IWU)) {
      cNode->states[STATE_CREST_SM] = cNode->params[PARAM_CREST_IWU] *
                                      cNode->params[PARAM_CREST_WM] / 100.0;
    }

    if (cNode->params[PARAM_CREST_WM] < 0.0) {
      cNode->params[PARAM_CREST_WM] = 100.0;
    }

    if (cNode->states[STATE_CREST_SM] < 0.0) {
      printf("Node Soil Moisture(%f) is less than 0, setting to 0.\n",
             cNode->states[STATE_CREST_SM]);
      cNode->states[STATE_CREST_SM] = 0.0;
    } else if (cNode->states[STATE_CREST_SM] > cNode->params[PARAM_CREST_WM]) {
      printf("Node Soil Moisture(%f) is greater than WM, setting to %f.\n",
             cNode->states[STATE_CREST_SM], cNode->params[PARAM_CREST_WM]);
    }

    if (cNode->params[PARAM_CREST_IM] < 0.0) {
      // printf("Node Impervious Area(%f) is less than 0, setting to 0.\n",
      // cNode->params[PARAM_CREST_IM]);
      cNode->params[PARAM_CREST_IM] = 0.0;
    } else if (cNode->params[PARAM_CREST_IM] > 1.0) {
      // printf("Node Impervious Area(%f) is greater than 1, setting to 1.\n",
      // cNode->params[PARAM_CREST_IM]);
      cNode->params[PARAM_CREST_IM] = 1.0;
    }

    if (cNode->params[PARAM_CREST_B] < 0.0) {
      // printf("Node B (%f) is less than 0, setting to 0.\n",
      // cNode->params[PARAM_CREST_B]);
      cNode->params[PARAM_CREST_B] = 1.0;
    } else if (cNode->params[PARAM_CREST_B] != cNode->params[PARAM_CREST_B]) {
      // printf("Node B (%f) NaN, setting to %f.\n",
      // cNode->params[PARAM_CREST_B], 0.0);
      cNode->params[PARAM_CREST_B] = 0.0;
    }

    if (cNode->params[PARAM_CREST_FC] < 0.0) {
      // printf("Node B (%f) is less than 0, setting to 0.\n",
      // cNode->params[PARAM_CREST_B]);
      cNode->params[PARAM_CREST_FC] = 1.0;
    }
  }
}

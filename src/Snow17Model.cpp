#include "Snow17Model.h"
#include "DatedName.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static const char *stateStrings[] = {
    "ati",
    "wq",
    "wi",
    "deficit",
};

Snow17Model::Snow17Model() {}

Snow17Model::~Snow17Model() {}

bool Snow17Model::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (snowNodes.size() != nodes->size()) {
    snowNodes.resize(nodes->size());
  }

  // Fill in modelIndex in the gridNodes
  size_t numNodes = nodes->size();
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    node->modelIndex = i;
    float elevation = g_DEM->data[node->y][node->x] / 100.0;
    snowNodes[i].P_atm = 33.86 * (29.9 - (0.335 * elevation) +
                                  (0.00022 * (powf(elevation, 2.4))));
    for (int p = 0; p < STATE_SNOW17_QTY; p++) {
      snowNodes[i].states[p] = 0.0;
    }
    // snowNodes[i].states[STATE_SNOW17_ATI] = 0.0;
    // snowNodes[i].states[STATE_SNOW17_WI] = 0.0;
    // snowNodes[i].states[STATE_SNOW17_WQ] = 0.0;
    // snowNodes[i].states[STATE_SNOW17_DEFICIT] = 0.0;
  }

  InitializeParameters(paramSettings, paramGrids);

  return true;
}

void Snow17Model::InitializeStates(TimeVar *beginTime, char *statePath) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(beginTime->GetTM());

  char buffer[300];
  for (int p = 0; p < STATE_SNOW17_QTY; p++) {
    sprintf(buffer, "%s/snow17_%s_%s.tif", statePath, stateStrings[p],
            timeStr.GetName());

    FloatGrid *sGrid = ReadFloatTifGrid(buffer);
    if (sGrid) {
      printf("Using Snow-17 %s State Grid %s\n", stateStrings[p], buffer);
      if (g_DEM->IsSpatialMatch(sGrid)) {
        for (size_t i = 0; i < nodes->size(); i++) {
          GridNode *node = &nodes->at(i);
          Snow17GridNode *cNode = &(snowNodes[i]);
          if (sGrid->data[node->y][node->x] != sGrid->noData) {
            cNode->states[p] = sGrid->data[node->y][node->x];
          }
        }
      } else {
        GridLoc pt;
        for (size_t i = 0; i < nodes->size(); i++) {
          GridNode *node = &(nodes->at(i));
          Snow17GridNode *cNode = &(snowNodes[i]);
          if (sGrid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) &&
              sGrid->data[pt.y][pt.x] != sGrid->noData) {
            cNode->states[p] = sGrid->data[pt.y][pt.x];
          }
        }
      }
      delete sGrid;
    } else {
      printf("Snow-17 %s State Grid %s not found!\n", stateStrings[p], buffer);
    }
  }
}

void Snow17Model::SaveStates(TimeVar *currentTime, char *statePath,
                             GridWriterFull *gridWriter) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(currentTime->GetTM());

  std::vector<float> dataVals;
  dataVals.resize(nodes->size());

  char buffer[300];
  for (int p = 0; p < STATE_SNOW17_QTY; p++) {
    sprintf(buffer, "%s/snow17_%s_%s.tif", statePath, stateStrings[p],
            timeStr.GetName());
    for (size_t i = 0; i < nodes->size(); i++) {
      Snow17GridNode *cNode = &(snowNodes[i]);
      dataVals[i] = cNode->states[p];
    }
    gridWriter->WriteGrid(nodes, &dataVals, buffer, false);
  }
}

bool Snow17Model::SnowBalance(float jday, float stepHours,
                              std::vector<float> *precip,
                              std::vector<float> *temp,
                              std::vector<float> *melt,
                              std::vector<float> *swe) {

  size_t numNodes = nodes->size();

  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    Snow17GridNode *cNode = &(snowNodes[i]);
    SnowBalanceInt(node, cNode, stepHours, jday, precip->at(i), temp->at(i),
                   &(melt->at(i)), &(swe->at(i)));
    // soilMoisture->at(i) = cNode->soilMoisture * 100.0 /
    // cNode->params[PARAM_CREST_WM];
  }

  return true;
}

void Snow17Model::SnowBalanceInt(GridNode *node, Snow17GridNode *cNode,
                                 float stepHours, float jday, float precipIn,
                                 float tempIn, float *melt, float *swe) {

  float stefan = 6.12e-10;
  float PXTEMP = 1; // Temperature of rainfall (Deg C)
  float TIPM_dtt =
      1.0 - (powf(1.0 - cNode->params[PARAM_SNOW17_TIPM], stepHours / 6));

  float precip = precipIn * stepHours; // precipIn is mm/hr, precip is mm

  float Sv =
      (0.5 * sinf((jday - 81 * 2 * M_PI) / 366.0)) + 0.5; // seasonal variation
  float mf = (stepHours / 6) *
             ((Sv * (cNode->params[PARAM_SNOW17_MFMAX] -
                     cNode->params[PARAM_SNOW17_MFMIN])) +
              cNode->params[PARAM_SNOW17_MFMIN]); // non-rain melt factor,
                                                  // seasonally varying

  // Snow Accumulation

  float fracsnow = 0.0;
  float fracrain = 1.0;

  if (tempIn < PXTEMP) {
    fracrain = 0.0;
    fracsnow = 1.0;
  }

  float Pn =
      precip * fracsnow *
      cNode->params[PARAM_SNOW17_SCF];  // water equivalent of new snowfall (mm)
  cNode->states[STATE_SNOW17_WI] += Pn; // W_i = accumulated water equivalent of
                                        // the ice portion of the snow cover
                                        // (mm)
  float E = 0;
  float RAIN =
      fracrain *
      precip; // amount of precip (mm) that is rain during this time step

  // Temperature and Heat Deficit from new Snow

  float T_snow_new = 0.0;
  float delta_HD_snow = 0.0;
  float T_rain = tempIn;

  if (tempIn < 0.0) {
    T_snow_new = tempIn;
    delta_HD_snow = -(T_snow_new * Pn) / (80.0 / 0.5); // delta_HD_snow = change
                                                       // in the heat deficit
                                                       // due to snowfall (mm)
    T_rain = PXTEMP;
  }

  // Antecedent temperature Index

  if (Pn > (1.5 * stepHours)) {
    cNode->states[STATE_SNOW17_ATI] = T_snow_new;
  } else {
    cNode->states[STATE_SNOW17_ATI] =
        cNode->states[STATE_SNOW17_ATI] +
        TIPM_dtt *
            (tempIn -
             cNode->states[STATE_SNOW17_ATI]); // Antecedent temperature index
  }

  if (cNode->states[STATE_SNOW17_ATI] > 0) {
    cNode->states[STATE_SNOW17_ATI] = 0;
  }

  // Heat Exchange when no Surface Melt

  float delta_HD_T = cNode->params[PARAM_SNOW17_NMF] * (stepHours / 6.0) *
                     (mf / cNode->params[PARAM_SNOW17_MFMAX]) *
                     (cNode->states[STATE_SNOW17_ATI] -
                      T_snow_new); // delta_HD_T = change in heat deficit due to
                                   // a temperature gradient (mm)

  // Rain-on-Snow Melt
  float M_RoS = 0.0;
  float e_sat =
      2.7489 * 100000000.0 *
      expf(
          (-4278.63 /
           (tempIn + 242.792))); // saturated vapor pressure at T_air_meanC (mb)
  if (RAIN > (0.25 * stepHours)) { // 1.5 mm/ 6 hrs
    // Melt (mm) during rain-on-snow periods is:
    float M_RoS1 = fmaxf(stefan * stepHours *
                             (powf(tempIn + 273.0, 4.0) - powf(273.0, 4.0)),
                         0.0);
    float M_RoS2 = fmaxf((0.0125 * RAIN * T_rain), 0.0);
    float M_RoS3 =
        fmaxf((8.5 * cNode->params[PARAM_SNOW17_UADJ] * (stepHours / 6) *
               (((0.9 * e_sat) - 6.11) + (0.00057 * cNode->P_atm * tempIn))),
              0.0);
    M_RoS = M_RoS1 + M_RoS2 + M_RoS3;
  }

  // Non-Rain Melt
  float M_NR = 0.0;
  if (RAIN <= (0.25 * stepHours) &&
      (tempIn > cNode->params[PARAM_SNOW17_MBASE])) {
    // Melt during non-rain periods is:
    M_NR = (mf * (tempIn - cNode->params[PARAM_SNOW17_MBASE])) +
           (0.0125 * RAIN * T_rain);
  }

  // Ripeness of the snow cover

  float Melt = M_RoS + M_NR;

  if (Melt < 0.0) {
    Melt = 0.0;
  }

  if (Melt < cNode->states[STATE_SNOW17_WI]) {
    cNode->states[STATE_SNOW17_WI] -= Melt;
  } else {
    Melt = cNode->states[STATE_SNOW17_WI] + cNode->states[STATE_SNOW17_WQ];
    cNode->states[STATE_SNOW17_WI] = 0;
  }

  float Qw = Melt + RAIN; // Qw = liquid water available melted/rained at the
                          // snow surface (mm)
  float W_qx =
      cNode->params[PARAM_SNOW17_PLWHC] *
      cNode->states[STATE_SNOW17_WI]; // W_qx = liquid water capacity (mm)
  cNode->states[STATE_SNOW17_DEFICIT] =
      cNode->states[STATE_SNOW17_DEFICIT] + delta_HD_snow +
      delta_HD_T; // Deficit = heat deficit (mm)

  if (cNode->states[STATE_SNOW17_DEFICIT] <= 0.0) { // limits of heat deficit
    cNode->states[STATE_SNOW17_DEFICIT] = 0.0;
  } else if (cNode->states[STATE_SNOW17_DEFICIT] >
             (0.33 * cNode->states[STATE_SNOW17_WI])) {
    cNode->states[STATE_SNOW17_DEFICIT] = 0.33 * cNode->states[STATE_SNOW17_WI];
  }

  float SWE = 0.0;
  // In SNOW-17 the snow cover is ripe when both (Deficit=0) & (W_q = W_qx)
  if (cNode->states[STATE_SNOW17_WI] > 0) {
    if ((Qw + cNode->states[STATE_SNOW17_WQ]) >
        ((cNode->states[STATE_SNOW17_DEFICIT] *
          (1 + cNode->params[PARAM_SNOW17_PLWHC])) +
         W_qx)) { // THEN the snow is RIPE
      E = Qw + cNode->states[STATE_SNOW17_WQ] - W_qx -
          (cNode->states[STATE_SNOW17_DEFICIT] *
           (1 + cNode->params[PARAM_SNOW17_PLWHC])); // Excess liquid water (mm)
      cNode->states[STATE_SNOW17_WQ] = W_qx; // fills liquid water capacity
      cNode->states[STATE_SNOW17_WI] =
          cNode->states[STATE_SNOW17_WI] +
          cNode->states[STATE_SNOW17_DEFICIT]; // W_i increases because water
                                               // refreezes as heat deficit is
                                               // decreased
      cNode->states[STATE_SNOW17_DEFICIT] = 0;
    } else if (
        (Qw >=
         cNode->states
             [STATE_SNOW17_DEFICIT])) { // %& ((Qw + cNode->W_q) <=
                                        // ((cNode->Deficit*(1+cNode->params[PARAM_SNOW17_PLWHC]))
                                        // + W_qx))) { // THEN the snow is NOT
                                        // yet ripe, but ice is being melted
      E = 0;
      cNode->states[STATE_SNOW17_WQ] = cNode->states[STATE_SNOW17_WQ] + Qw -
                                       cNode->states[STATE_SNOW17_DEFICIT];
      cNode->states[STATE_SNOW17_WI] =
          cNode->states[STATE_SNOW17_WI] +
          cNode->states[STATE_SNOW17_DEFICIT]; // W_i increases because water
                                               // refreezes as heat deficit is
                                               // decreased
      cNode->states[STATE_SNOW17_DEFICIT] = 0;
    } else if ((Qw <
                cNode->states[STATE_SNOW17_DEFICIT])) { // elseif ((Qw + W_q) <
                                                        // cNode->Deficit)) { //
                                                        // THEN the snow is NOT
                                                        // yet ripe
      E = 0;
      cNode->states[STATE_SNOW17_WI] += Qw; // W_i increases because water
                                            // refreezes as heat deficit is
                                            // decreased
      cNode->states[STATE_SNOW17_DEFICIT] -= Qw;
    }

    SWE =
        cNode->states[STATE_SNOW17_WI] + cNode->states[STATE_SNOW17_WQ]; // + E;
  } else {
    // then no snow exists!
    E = Qw;
    SWE = 0;
    cNode->states[STATE_SNOW17_WQ] = 0;
  }

  if (cNode->states[STATE_SNOW17_DEFICIT] == 0) {
    cNode->states[STATE_SNOW17_ATI] = 0;
  }

  // End of model execution

  /*if (SWE == 0.0) {
          printf("pIn %f, pOut %f\n", cNode->pIn, cNode->pOut);
  }*/

  *swe = SWE; // total SWE (mm) at this time step
  *melt = E / stepHours;
}

void Snow17Model::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  size_t numNodes = nodes->size();
  size_t unused = 0;
  for (size_t i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    Snow17GridNode *cNode = &(snowNodes[i]);
    if (!node->gauge) {
      unused++;
      continue;
    }
    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_SNOW17_QTY);

    // Some of the parameters are special, deal with that here
    /*if (!paramGrids->at(PARAM_CREST_IM)) {
     cNode->params[PARAM_CREST_IM] /= 100.0;
     }*/

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_SNOW17_QTY; paramI++) {
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

    /*if (!paramGrids->at(PARAM_CREST_IWU)) {
      cNode->soilMoisture = cNode->params[PARAM_CREST_IWU] *
    cNode->params[PARAM_CREST_WM] / 100.0;
    }

    if (cNode->soilMoisture < 0.0) {
      printf("Node Soil Moisture(%f) is less than 0, setting to 0.\n",
    cNode->soilMoisture); cNode->soilMoisture = 0.0; } else if
    (cNode->soilMoisture > cNode->params[PARAM_CREST_WM]) { printf("Node Soil
    Moisture(%f) is greater than WM, setting to %f.\n", cNode->soilMoisture,
    cNode->params[PARAM_CREST_WM]);
    }

    if (cNode->params[PARAM_CREST_IM] < 0.0) {
      printf("Node Impervious Area(%f) is less than 0, setting to 0.\n",
    cNode->params[PARAM_CREST_IM]); cNode->params[PARAM_CREST_IM] = 0.0; } else
    if (cNode->params[PARAM_CREST_IM] > 1.0) { printf("Node Impervious Area(%f)
    is greater than 1, setting to 1.\n", cNode->params[PARAM_CREST_IM]);
      cNode->params[PARAM_CREST_IM] = 1.0;
    }

    if (cNode->params[PARAM_CREST_B] < 0.0) {
      printf("Node B (%f) is less than 0, setting to 0.\n",
    cNode->params[PARAM_CREST_B]); cNode->params[PARAM_CREST_B] = 0.0; } else if
    (cNode->params[PARAM_CREST_B] != cNode->params[PARAM_CREST_B]) {
      printf("Node B (%f) NaN, setting to %f.\n", cNode->params[PARAM_CREST_B],
    0.0); cNode->params[PARAM_CREST_B] = 0.0;
    }*/
  }
}

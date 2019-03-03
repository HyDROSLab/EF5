#include "SAC.h"
#include "DatedName.h"
#include <cmath>
#include <cstdio>
#include <cstring>

SAC::SAC() {}

SAC::~SAC() {}

bool SAC::InitializeModel(
    std::vector<GridNode> *newNodes,
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  nodes = newNodes;
  if (sacNodes.size() != nodes->size()) {
    sacNodes.resize(nodes->size());
  }

  // Fill in modelNode in the gridNodes
  std::vector<SACGridNode>::iterator citr = sacNodes.begin();
  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    SACGridNode *cNode = &(*citr);

    node->modelNode = cNode;

    citr++;
  }

  InitializeParameters(paramSettings, paramGrids);

  return true;
}

void SAC::InitializeStates(TimeVar *beginTime, char *statePath) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(beginTime->GetTM());

  char buffer[300];
  sprintf(buffer, "%s/uztwc_%s.tif", statePath, timeStr.GetName());
  FloatGrid *smGrid = ReadFloatTifGrid(buffer);
  if (smGrid) {
    if (g_DEM->IsSpatialMatch(smGrid)) {
      printf("Using Previous UZTWC Grid %s\n", buffer);
      for (size_t i = 0; i < nodes->size(); i++) {
        GridNode *node = &nodes->at(i);
        SACGridNode *cNode = &(sacNodes[i]);
        if (smGrid->data[node->y][node->x] != smGrid->noData) {
          cNode->UZTWC = smGrid->data[node->y][node->x];
        }
      }
    } else {
      printf("Previous UZTWC Grid %s not a spatial match!\n", buffer);
    }
    delete smGrid;
  } else {
    printf("Previous UZTWC Grid %s not found!\n", buffer);
  }

  sprintf(buffer, "%s/uzfwc_%s.tif", statePath, timeStr.GetName());
  smGrid = ReadFloatTifGrid(buffer);
  if (smGrid) {
    if (g_DEM->IsSpatialMatch(smGrid)) {
      printf("Using Previous UZFWC Grid %s\n", buffer);
      for (size_t i = 0; i < nodes->size(); i++) {
        GridNode *node = &nodes->at(i);
        SACGridNode *cNode = &(sacNodes[i]);
        if (smGrid->data[node->y][node->x] != smGrid->noData) {
          cNode->UZFWC = smGrid->data[node->y][node->x];
        }
      }
    } else {
      printf("Previous UZFWC Grid %s not a spatial match!\n", buffer);
    }
    delete smGrid;
  } else {
    printf("Previous UZFWC Grid %s not found!\n", buffer);
  }

  sprintf(buffer, "%s/lztwc_%s.tif", statePath, timeStr.GetName());
  smGrid = ReadFloatTifGrid(buffer);
  if (smGrid) {
    if (g_DEM->IsSpatialMatch(smGrid)) {
      printf("Using Previous LZTWC Grid %s\n", buffer);
      for (size_t i = 0; i < nodes->size(); i++) {
        GridNode *node = &nodes->at(i);
        SACGridNode *cNode = &(sacNodes[i]);
        if (smGrid->data[node->y][node->x] != smGrid->noData) {
          cNode->LZTWC = smGrid->data[node->y][node->x];
        }
      }
    } else {
      printf("Previous LZTWC Grid %s not a spatial match!\n", buffer);
    }
    delete smGrid;
  } else {
    printf("Previous LZTWC Grid %s not found!\n", buffer);
  }

  sprintf(buffer, "%s/lzfsc_%s.tif", statePath, timeStr.GetName());
  smGrid = ReadFloatTifGrid(buffer);
  if (smGrid) {
    if (g_DEM->IsSpatialMatch(smGrid)) {
      printf("Using Previous LZFSC Grid %s\n", buffer);
      for (size_t i = 0; i < nodes->size(); i++) {
        GridNode *node = &nodes->at(i);
        SACGridNode *cNode = &(sacNodes[i]);
        if (smGrid->data[node->y][node->x] != smGrid->noData) {
          cNode->LZFSC = smGrid->data[node->y][node->x];
        }
      }
    } else {
      printf("Previous LZFSC Grid %s not a spatial match!\n", buffer);
    }
    delete smGrid;
  } else {
    printf("Previous LZFSC Grid %s not found!\n", buffer);
  }

  sprintf(buffer, "%s/lzfpc_%s.tif", statePath, timeStr.GetName());
  smGrid = ReadFloatTifGrid(buffer);
  if (smGrid) {
    if (g_DEM->IsSpatialMatch(smGrid)) {
      printf("Using Previous LZFPC Grid %s\n", buffer);
      for (size_t i = 0; i < nodes->size(); i++) {
        GridNode *node = &nodes->at(i);
        SACGridNode *cNode = &(sacNodes[i]);
        if (smGrid->data[node->y][node->x] != smGrid->noData) {
          cNode->LZFPC = smGrid->data[node->y][node->x];
        }
      }
    } else {
      printf("Previous LZFPC Grid %s not a spatial match!\n", buffer);
    }
    delete smGrid;
  } else {
    printf("Previous LZFPC Grid %s not found!\n", buffer);
  }

  sprintf(buffer, "%s/adimc_%s.tif", statePath, timeStr.GetName());
  smGrid = ReadFloatTifGrid(buffer);
  if (smGrid) {
    if (g_DEM->IsSpatialMatch(smGrid)) {
      printf("Using Previous ADIMC Grid %s\n", buffer);
      for (size_t i = 0; i < nodes->size(); i++) {
        GridNode *node = &nodes->at(i);
        SACGridNode *cNode = &(sacNodes[i]);
        if (smGrid->data[node->y][node->x] != smGrid->noData) {
          cNode->ADIMC = smGrid->data[node->y][node->x];
        }
      }
    } else {
      printf("Previous ADIMC Grid %s not a spatial match!\n", buffer);
    }
    delete smGrid;
  } else {
    printf("Previous ADIMC Grid %s not found!\n", buffer);
  }
}

void SAC::SaveStates(TimeVar *currentTime, char *statePath,
                     GridWriterFull *gridWriter) {
  DatedName timeStr;
  timeStr.SetNameStr("YYYYMMDD_HHUU");
  timeStr.ProcessNameLoose(NULL);
  timeStr.UpdateName(currentTime->GetTM());

  std::vector<float> dataVals;
  dataVals.resize(nodes->size());

  // UZTWC, UZFWC, LZTWC, LZFSC, LZFPC, ADIMC

  char buffer[300];
  sprintf(buffer, "%s/uztwc_%s.tif", statePath, timeStr.GetName());
  for (size_t i = 0; i < nodes->size(); i++) {
    SACGridNode *cNode = &(sacNodes[i]);
    dataVals[i] = cNode->UZTWC;
  }
  gridWriter->WriteGrid(nodes, &dataVals, buffer, false);

  sprintf(buffer, "%s/uzfwc_%s.tif", statePath, timeStr.GetName());
  for (size_t i = 0; i < nodes->size(); i++) {
    SACGridNode *cNode = &(sacNodes[i]);
    dataVals[i] = cNode->UZFWC;
  }
  gridWriter->WriteGrid(nodes, &dataVals, buffer, false);

  sprintf(buffer, "%s/lztwc_%s.tif", statePath, timeStr.GetName());
  for (size_t i = 0; i < nodes->size(); i++) {
    SACGridNode *cNode = &(sacNodes[i]);
    dataVals[i] = cNode->LZTWC;
  }
  gridWriter->WriteGrid(nodes, &dataVals, buffer, false);

  sprintf(buffer, "%s/lzfsc_%s.tif", statePath, timeStr.GetName());
  for (size_t i = 0; i < nodes->size(); i++) {
    SACGridNode *cNode = &(sacNodes[i]);
    dataVals[i] = cNode->LZFSC;
  }
  gridWriter->WriteGrid(nodes, &dataVals, buffer, false);

  sprintf(buffer, "%s/lzfpc_%s.tif", statePath, timeStr.GetName());
  for (size_t i = 0; i < nodes->size(); i++) {
    SACGridNode *cNode = &(sacNodes[i]);
    dataVals[i] = cNode->LZFPC;
  }
  gridWriter->WriteGrid(nodes, &dataVals, buffer, false);

  sprintf(buffer, "%s/adimc_%s.tif", statePath, timeStr.GetName());
  for (size_t i = 0; i < nodes->size(); i++) {
    SACGridNode *cNode = &(sacNodes[i]);
    dataVals[i] = cNode->ADIMC;
  }
  gridWriter->WriteGrid(nodes, &dataVals, buffer, false);
}
bool SAC::WaterBalance(float stepHours, std::vector<float> *precip,
                       std::vector<float> *pet, std::vector<float> *fastFlow,
                       std::vector<float> *slowFlow,
                       std::vector<float> *soilMoisture) {

  size_t numNodes = nodes->size();
  size_t i = 0;

#if _OPENMP
  //#pragma omp parallel for
#endif
  for (i = 0; i < numNodes; i++) {
    GridNode *node = &nodes->at(i);
    if (!node->gauge) {
      continue;
    }
    SACGridNode *cNode = (SACGridNode *)node->modelNode;
    WaterBalanceInt(node, cNode, stepHours, precip->at(i), pet->at(i));
    fastFlow->at(i) += (cNode->dischargeF / (stepHours * 3600.0f));
    slowFlow->at(i) += (cNode->dischargeS / (stepHours * 3600.0f));
    soilMoisture->at(i) =
        100.0 * (cNode->UZTWC + cNode->UZFWC) /
        (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_UZFWM]);
    if (!std::isfinite(soilMoisture->at(i))) {
      soilMoisture->at(i) = 0;
    }
    // soilMoisture->at(i) = (cNode->UZTWC + cNode->UZFWC) /
    // (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_UZFWM]);
    // discharge->at(i) = (cNode->dischargeF + cNode->dischargeS) * node->area *
    // 0.277777777777778f; // Convert from mm/time to cms;  printf(" Q %f\n",
    // discharge->at(i));  LocalRouteQF(node, cNode);  LocalRouteSF(node, cNode);
  }

  return true;
}

void SAC::WaterBalanceInt(GridNode *node, SACGridNode *cNode, float stepHours,
                          float precipIn, float petIn) {

  /*float precip = 0.0f; //precipIn * stepHours; // precipIn is mm/hr, precip is
mm float pet = 20.0f; //petIn * stepHours; // petIn in mm/hr, pet is mm float DT
= 3.0f / 24.0f; //stepHours / 24.0f;*/

  float precip =
      precipIn *
      stepHours; // stepHours; //stepHours; // precipIn is mm/hr, precip is mm
  float pet = petIn * stepHours; // * 0.06; // petIn in mm/hr, pet is mm
  float DT = stepHours / 24.0f;

  float PAREA =
      1.0f - cNode->params[PARAM_SAC_PCTIM] - cNode->params[PARAM_SAC_ADIMP];

  /*******************************/
  /*       ET Calculations       */
  /*******************************/

  float E1 = 0.0f;  // ET from upper zone
  float RED = 0.0f; // Residual ET demand
  float E2 = 0.0f;  // ET from UZFWC
  float E3 = 0.0f;  // ET from lower zone (LZTWC)
  float E5 = 0.0f;  // ET from ADIMP area

  E1 = pet *
       (cNode->UZTWC / cNode->params[PARAM_SAC_UZTWM]); // ET from upper zone
  RED = pet - E1;                                       // Residual ET demand
  cNode->UZTWC -= E1;
  if (cNode->UZTWC < 0.0f) { // E1 can't exceed UZTWC
    E1 += cNode->UZTWC;
    cNode->UZTWC = 0.0f;
    RED = pet - E1;
    if (cNode->UZFWC < RED) {
      E2 = cNode->UZFWC;
      cNode->UZFWC = 0.0f;
      RED = RED - E2;
    } else {
      E2 = RED;
      cNode->UZFWC -= E2;
      RED = 0.0f;
    }
  }

  if ((cNode->UZTWC / cNode->params[PARAM_SAC_UZTWM]) <
      (cNode->UZFWC / cNode->params[PARAM_SAC_UZFWM])) {
    // Upper zone free water ratio exceeds upper zone tension
    // water ratio, thus transfer free water to tension
    float UZRAT =
        (cNode->UZTWC + cNode->UZFWC) /
        (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_UZFWM]);
    cNode->UZTWC = cNode->params[PARAM_SAC_UZTWM] * UZRAT;
    cNode->UZFWC = cNode->params[PARAM_SAC_UZFWM] * UZRAT;
  }

  if (cNode->UZTWC < 0.00001f) {
    cNode->UZTWC = 0.0f;
  }

  if (cNode->UZFWC < 0.00001f) {
    cNode->UZFWC = 0.0f;
  }

  E3 = RED * (cNode->LZTWC / (cNode->params[PARAM_SAC_UZTWM] +
                              cNode->params[PARAM_SAC_LZTWM]));
  cNode->LZTWC -= E3;

  if (cNode->LZTWC < 0.0f) {
    // E3 can't exceed LZTWC
    E3 += cNode->LZTWC;
    cNode->LZTWC = 0.0f;
  }

  float RATLZT = cNode->LZTWC / cNode->params[PARAM_SAC_LZTWM];
  float RATLZ =
      (cNode->LZTWC + cNode->LZFPC + cNode->LZFSC -
       cNode->params[PARAM_SAC_RSERV]) /
      (cNode->params[PARAM_SAC_LZTWM] + cNode->params[PARAM_SAC_LZFPM] +
       cNode->params[PARAM_SAC_LZFSM] - cNode->params[PARAM_SAC_RSERV]);

  if (RATLZT < RATLZ) {
    // Resupply lower zone tension water from lower
    // zone free water if more water available there.
    float DEL = (RATLZ - RATLZT) * cNode->params[PARAM_SAC_LZTWM];
    cNode->LZTWC += DEL;
    cNode->LZFSC -= DEL;
    if (cNode->LZFSC < 0.0f) {
      // If transfer exceeds LZFSC then remainder comes from LZFPC
      cNode->LZFPC += cNode->LZFSC;
      cNode->LZFSC = 0.0f;
    }
  }

  if (cNode->LZTWC < 0.00001f) {
    cNode->LZTWC = 0.0f;
  }

  E5 = E1 + (RED + E2) * ((cNode->ADIMC - E1 - cNode->UZTWC) /
                          (cNode->params[PARAM_SAC_UZTWM] +
                           cNode->params[PARAM_SAC_LZTWM]));
  // Adjust adimc, additional impervious area storage, for evaporation
  cNode->ADIMC -= E5;

  if (cNode->ADIMC < 0.0f) {
    E5 += cNode->ADIMC;
    cNode->ADIMC = 0.0f;
  }

  E5 *= cNode->params[PARAM_SAC_ADIMP];

  /**********************************************/
  /*    Compute Percolation & runoff amounts    */
  /**********************************************/

  // TWX is the time interval available moisture in excess of UZTW requirements
  float TWX = precip + cNode->UZTWC - cNode->params[PARAM_SAC_UZTWM];

  if (TWX < 0.0f) {
    // All moisture held in UZTW -- No excess.
    cNode->UZTWC += precip;
    TWX = 0.0f;
  } else {
    cNode->UZTWC =
        cNode->params[PARAM_SAC_UZTWM]; // Moisture in excess of UZTW storage
  }

  cNode->ADIMC = cNode->ADIMC + precip - TWX;

  // Compute impervious area runoff
  float ROIMP =
      precip *
      cNode->params[PARAM_SAC_PCTIM]; // Runoff from minimum impervious area

  float SBF = 0.0f, SSUR = 0.0f, SIF = 0.0f, SPERC = 0.0f, SDRO = 0.0f,
        SPBF = 0.0f;

  float NINC =
      floor(1.0f + 0.2f * (cNode->UZFWC + TWX)); // Number of time increments
                                                 // that the time interval is
                                                 // divided into for further
                                                 // soil-moisture accounting.
  // No one increment will exceed 5.0 millimeters of UZFWC+PAV

  float DINC = (1.0f / NINC) * DT; // Length of each increment in days

  float PINC = TWX / NINC; // Amount of available moisture for each increment.
                           // Compute free water depletion fractions for the
                           // time increment
                           // being used-basic depletions are for one day

  float DUZ = 1.0f - pow(1.0f - cNode->params[PARAM_SAC_UZK], DINC);
  float DLZP = 1.0f - pow(1.0f - cNode->params[PARAM_SAC_LZPK], DINC);
  float DLZS = 1.0f - pow(1.0f - cNode->params[PARAM_SAC_LZSK], DINC);

  /*******************************************/
  /*        Do loop for time interval        */
  /*******************************************/

  for (float i = 0.0f; i < NINC; i = i + 1.0f) {
    float ADSUR = 0.0f;
    float RATIO =
        (cNode->ADIMC - cNode->UZTWC) / cNode->params[PARAM_SAC_LZTWM];
    if (RATIO < 0.0f) {
      RATIO = 0.0f;
    }

    float ADDRO =
        PINC * pow(RATIO, 2.0f); // Amount of direct runoff from the area ADIMP

    // Compute baseflow
    float BF = cNode->LZFPC * DLZP;
    cNode->LZFPC -= BF;
    if (cNode->LZFPC <= 0.0001f) {
      BF += cNode->LZFPC;
      cNode->LZFPC = 0.0f;
    }

    SBF += BF;
    SPBF += BF;

    BF = cNode->LZFSC * DLZS;
    cNode->LZFSC -= BF;
    if (cNode->LZFSC <= 0.0001f) {
      BF += cNode->LZFSC;
      cNode->LZFSC = 0.0f;
    }

    SBF += BF;

    // Compute Percolation, If no water is available then skip
    if ((PINC + cNode->UZFWC) <= 0.01f) {
      /*cNode->UZFWC += PINC;
cNode->ADIMC = cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM];
SDRO = SDRO + ADDRO * cNode->params[PARAM_SAC_ADIMP];
if (cNode->ADIMC < 0.00001f) {
cNode->ADIMC = 0.0f;
}*/

      cNode->ADIMC = cNode->ADIMC + PINC - ADDRO - ADSUR;
      if (cNode->ADIMC >
          (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM])) {
        ADDRO =
            ADDRO + cNode->ADIMC -
            (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM]);
        cNode->ADIMC =
            cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM];
      }
      SDRO = SDRO + ADDRO * cNode->params[PARAM_SAC_ADIMP];
      if (cNode->ADIMC < 0.00001f) {
        cNode->ADIMC = 0.0f;
      }
      continue;
    }

    float PERCM = cNode->params[PARAM_SAC_LZFPM] * DLZP +
                  cNode->params[PARAM_SAC_LZFSM] * DLZS;
    float PERC = PERCM * (cNode->UZFWC / cNode->params[PARAM_SAC_UZFWM]);
    // DEFR is the lower zone moisture deficiency ratio
    float DEFR = 1.0f - ((cNode->LZTWC + cNode->LZFPC + cNode->LZFSC) /
                         (cNode->params[PARAM_SAC_LZTWM] +
                          cNode->params[PARAM_SAC_LZFPM] +
                          cNode->params[PARAM_SAC_LZFSM]));
    float FR = 1.0f; // Change in percolation withdrawal due to frozen ground.
    float FI = 1.0f; // Change in interflow withdrawal due to frozen ground.
    // float IFRZE = 0.0f;

    PERC = PERC *
           (1.0f + cNode->params[PARAM_SAC_ZPERC] *
                       pow(DEFR, cNode->params[PARAM_SAC_REXP])) *
           FR;
    // Note... percolation occurs from UZFWC before PAV is added.

    if (PERC >= cNode->UZFWC) {
      PERC = cNode->UZFWC;
    }

    cNode->UZFWC -= PERC;

    // Check to see if percolation exceeds lower zone deficiency
    float CHECK = cNode->LZTWC + cNode->LZFPC + cNode->LZFSC + PERC -
                  cNode->params[PARAM_SAC_LZTWM] -
                  cNode->params[PARAM_SAC_LZFPM] -
                  cNode->params[PARAM_SAC_LZFSM];
    if (CHECK > 0.0f) {
      PERC -= CHECK;
      cNode->UZFWC += CHECK;
    }

    SPERC += PERC; // Time interval summation of PERC

    // Compute interflow and keep track of time interval sum
    // Note PINC has not yet been added
    float DEL = cNode->UZFWC * DUZ * FI;
    SIF += DEL;
    cNode->UZFWC -= DEL;

    // Distribute percolated water into the lower zones
    // tension water must be filled first except for the PFREE area.
    // PERCT is percolation to tension water and PERCF is percolation going to
    // free water
    float PERCT = PERC * (1.0f - cNode->params[PARAM_SAC_PFREE]);
    float PERCF;
    if ((PERCT + cNode->LZTWC) <= cNode->params[PARAM_SAC_LZTWM]) {
      cNode->LZTWC += PERCT;
      PERCF = 0.0f;
    } else {
      PERCF = PERCT + cNode->LZTWC - cNode->params[PARAM_SAC_LZTWM];
      cNode->LZTWC = cNode->params[PARAM_SAC_LZTWM];
    }

    // Distribute percolation in excess of tension requirements among the
    // free water storages
    PERCF += (PERC * cNode->params[PARAM_SAC_PFREE]);
    if (PERCF != 0.0f) {
      float HPL =
          cNode->params[PARAM_SAC_LZFPM] /
          (cNode->params[PARAM_SAC_LZFPM] + cNode->params[PARAM_SAC_LZFSM]);
      // HPL is the relative size of the primary storage
      // as compared with total lower zone free water storage

      float RATLP = cNode->LZFPC / cNode->params[PARAM_SAC_LZFPM];
      float RATLS = cNode->LZFSC / cNode->params[PARAM_SAC_LZFSM];
      // RATLP and RATLS are content capacit ratios, or in other words
      // the relative fullness of each storage

      float FRACP =
          (HPL * 2.0f * (1.0f - RATLP)) / ((1.0f - RATLP) + (1.0f - RATLS));
      // FRACP is the fraction going to primary
      if (FRACP > 1.0f) {
        FRACP = 1.0f;
      }

      float PERCP = PERCF * FRACP;
      float PERCS = PERCF - PERCP;
      // PERCP and PERCS are the amount of excess percolation
      // going to primary and supplemental storages, respectively

      cNode->LZFSC += PERCS;
      if (cNode->LZFSC > cNode->params[PARAM_SAC_LZFSM]) {
        PERCS = PERCS - cNode->LZFSC + cNode->params[PARAM_SAC_LZFSM];
        cNode->LZFSC = cNode->params[PARAM_SAC_LZFSM];
      }

      cNode->LZFPC = cNode->LZFPC + PERCF - PERCS;
      if (cNode->LZFPC > cNode->params[PARAM_SAC_LZFPM]) {
        float EXCESS = cNode->LZFPC - cNode->params[PARAM_SAC_LZFPM];
        cNode->LZTWC += EXCESS;
        cNode->LZFPC = cNode->params[PARAM_SAC_LZFPM];
      }
    }

    // Distribute PINC between UZFWC & surface runoff
    if (PINC != 0.0f) {
      if ((PINC + cNode->UZFWC) <= cNode->params[PARAM_SAC_UZFWM]) {
        // No surface runoff
        cNode->UZFWC += PINC;
      } else {
        float SUR = PINC + cNode->UZFWC - cNode->params[PARAM_SAC_UZFWM];
        SSUR = SSUR + SUR * PAREA;

        ADSUR = SUR * (1.0f - ADDRO / PINC);
        // ADSUR is the amount of surface runoff which comes from
        // that portion of ADIMP which is not
        // currently generating direct runoff. ADDRO/PINC is the fraction
        // of ADIMP currently generating direct runoff.

        SSUR = SSUR + ADSUR * cNode->params[PARAM_SAC_ADIMP];
      }
    }

    // ADIMP area water balance -- SDRO is the IDT sum of the direct runoff
    cNode->ADIMC = cNode->ADIMC + PINC - ADDRO - ADSUR;
    if (cNode->ADIMC >
        (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM])) {
      ADDRO = ADDRO + cNode->ADIMC -
              (cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM]);
      cNode->ADIMC =
          cNode->params[PARAM_SAC_UZTWM] + cNode->params[PARAM_SAC_LZTWM];
    }

    SDRO = SDRO + ADDRO * cNode->params[PARAM_SAC_ADIMP];
    if (cNode->ADIMC < 0.00001f) {
      cNode->ADIMC = 0.0f;
    }
  }

  /****************************************/
  /*       End of incremental Loop        */
  /****************************************/

  // Compute sums and adjust runoff amounts by the area over which they are
  // generated

  float EUSED = E1 + E2 + E3; // ET from PAREA which is 1.0 - ADIMP - PCTIM

  SIF *= PAREA;

  // Separate channel component of baseflow from the non-channel component
  float TBF = SBF * PAREA; // Total baseflow
  float BFCC =
      TBF *
      (1.0f /
       (1.0f + cNode->params[PARAM_SAC_SIDE])); // Baseflow, channel component

  float BFP = SPBF * PAREA / (1.0f + cNode->params[PARAM_SAC_SIDE]);
  float BFS = BFCC - BFP;
  if (BFS < 0.0f) {
    BFS = 0.0f;
  }
  // float BFNCC = TBF - BFCC; // Baseflow, non-channel component

  float TCI = ROIMP + SDRO + SSUR + SIF +
              BFCC;        // Total channel inflow for the time interval
  float GRND = SIF + BFCC; // interflow part of ground flow
  float SURF = TCI - GRND; // interflow part of surface flow

  float E4 = (pet - EUSED) * cNode->params[PARAM_SAC_RIVA]; // / stepHours; //
                                                            // ET from Riparian
                                                            // vegetation

  static int II = 0;
  // printf(" **** %i ****\n",II);
  II++;
  // printf(" %f %f %f %f %f %f %f\n", TCI, ROIMP, SDRO, SSUR, SIF, BFCC, NINC);

  TCI -= E4;
  if (TCI < 0.0f) {
    E4 += TCI;
    TCI = 0.0f;
  }

  GRND -= E4;
  if (GRND < 0.0f) {
    SURF += GRND;
    GRND = 0.0f;
    if (SURF < 0.0f) {
      SURF = 0.0f;
    }
  }

  EUSED *= PAREA;
  // float TET = EUSED + E5 + E4; // total evaportranspiration
  if (cNode->ADIMC < cNode->UZTWC) {
    cNode->ADIMC = cNode->UZTWC;
  }

  cNode->dischargeF = SURF;
  cNode->dischargeS = GRND;

  // printf(" %f %f %f %f %f\n", EUSED, E5, E4, pet,
  // cNode->params[PARAM_SAC_RIVA]);  printf(" %f %f %f\n", SURF, GRND, TET);
  // printf("1: %f %f %f %f %f %f", cNode->UZTWC, cNode->UZFWC, cNode->LZTWC,
  // cNode->LZFSC, cNode->LZFPC, cNode->ADIMC);
}

void SAC::InitializeParameters(
    std::map<GaugeConfigSection *, float *> *paramSettings,
    std::vector<FloatGrid *> *paramGrids) {

  // This pass distributes parameters
  for (std::vector<GridNode>::iterator itr = nodes->begin();
       itr != nodes->end(); itr++) {
    GridNode *node = &(*itr);
    SACGridNode *cNode = (SACGridNode *)node->modelNode;
    if (!node->gauge) {
      continue;
    }

    // Copy all of the parameters over
    memcpy(cNode->params, (*paramSettings)[node->gauge],
           sizeof(float) * PARAM_SAC_QTY);

    // Initialize states
    cNode->UZTWC = cNode->params[PARAM_SAC_UZTWC] *
                   cNode->params[PARAM_SAC_UZTWM]; // 0.0f;
    cNode->UZFWC = cNode->params[PARAM_SAC_UZFWC] *
                   cNode->params[PARAM_SAC_UZFWM]; // 0.0f; //2.773;
    cNode->LZTWC = cNode->params[PARAM_SAC_LZTWC] *
                   cNode->params[PARAM_SAC_LZTWM]; // 0.0f; //286.7;
    cNode->LZFSC = cNode->params[PARAM_SAC_LZFSC] *
                   cNode->params[PARAM_SAC_LZFSM]; // 0.0f;
    cNode->LZFPC = cNode->params[PARAM_SAC_LZFPC] *
                   cNode->params[PARAM_SAC_LZFPM]; // 0.0f; //154.2;
    cNode->ADIMC = cNode->params[PARAM_SAC_ADIMC]; //:x0.0f;

    // Deal with the distributed parameters here
    GridLoc pt;
    for (size_t paramI = 0; paramI < PARAM_CREST_QTY; paramI++) {
      FloatGrid *grid = paramGrids->at(paramI);
      if (grid && grid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt)) {
        if (grid->data[pt.y][pt.x] != grid->noData) {
          cNode->params[paramI] *= grid->data[pt.y][pt.x];
        }
      }
    }
  }
}

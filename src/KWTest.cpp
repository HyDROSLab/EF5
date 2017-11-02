#include <cstdio>

#include "Defines.h"
#include "EF5.h"
#include "GaugeConfigSection.h"
#include "GridNode.h"
#include "KinematicRoute.h"
#include "Model.h"
#include "ModelBase.h"

#define NUM_GRID_CELLS 101
#define CELL_SIZE (200.0)
#define TOTAL_TIME_STEPS 1000

void PrintStartupMessage();
void InitializeKW();
void SimulateRouting();

std::vector<GridNode> nodes;
RoutingModel *rModel;
std::map<GaugeConfigSection *, float *> fullParamSettingsRoute;
std::vector<float> currentFF, currentSF, currentQ;
std::vector<FloatGrid *> paramGridsRoute;
GaugeConfigSection gaugeConfigSec("010000");
float params[PARAM_KINEMATIC_QTY];

float inflowHydrograph[] = {0.0,        56.633694,  56.633694,  84.950541,
                            113.267388, 141.584235, 169.901082, 141.584235,
                            113.267388, 84.950541,  56.633694,  56.633694};

int main(int argc, char *argv[]) {

  PrintStartupMessage();
  InitializeKW();
  SimulateRouting();

  return ERROR_SUCCESS;
}

void InitializeKW() {

  rModel = new KWRoute();

  nodes.resize(NUM_GRID_CELLS);
  currentQ.resize(nodes.size());
  currentSF.resize(nodes.size());
  currentFF.resize(nodes.size());
  for (int currentNode = 0; currentNode < NUM_GRID_CELLS; currentNode++) {
    GridNode *currentN = &(nodes)[currentNode];

    // Setup the initial node for initiating the search for upstream nodes
    currentN->index = currentNode;
    currentN->x = currentNode;
    currentN->y = 0;
    if (currentNode == 0) {
      currentN->downStreamNode = INVALID_DOWNSTREAM_NODE;
    } else {
      currentN->downStreamNode = currentNode - 1;
    }
    currentN->horLen = CELL_SIZE; // meters
    currentN->slope = 0.01;       // 10.0 / currentN->horLen; // We assume a
                            // difference in height of 1 meter because we know
                            // nothing else
    currentN->area = (CELL_SIZE * CELL_SIZE) / 1000000.0; // km2
    currentN->fac = NUM_GRID_CELLS - currentNode + 1;
    currentN->gauge = &gaugeConfigSec;

    currentQ[currentNode] = 0.0;
    currentSF[currentNode] = 0.0;
    currentFF[currentNode] = 0.0;
  }

  // We only use lumped parameters here for ease of use.
  for (size_t paramI = 0; paramI < PARAM_KINEMATIC_QTY; paramI++) {
    paramGridsRoute.push_back(NULL);
  }

  // Parameter setup
  fullParamSettingsRoute[&gaugeConfigSec] = params;
  params[PARAM_KINEMATIC_COEM] = 50.0;
  params[PARAM_KINEMATIC_UNDER] = 0.1;
  params[PARAM_KINEMATIC_LEAKI] = 1.0;
  params[PARAM_KINEMATIC_TH] = -1.0;
  params[PARAM_KINEMATIC_ISU] = 0.0;
  params[PARAM_KINEMATIC_ALPHA] = 3.49;
  params[PARAM_KINEMATIC_BETA] = 0.60;

  rModel->InitializeModel(&nodes, &fullParamSettingsRoute, &paramGridsRoute);
}

void SimulateRouting() {
  FILE *file = fopen("results.csv", "w");
  fprintf(file, "%s", "Time,Inflow(m^3 s^-1),Outflow(m^3 s^-1)\n");
  float stepHoursReal = 5.0 / 60.0; /// 3600.0f;

  int numInflow = sizeof(inflowHydrograph) / sizeof(inflowHydrograph[0]);
  float totalIn = 0.0, totalOut = 0.0;
  for (int i = 0; i < TOTAL_TIME_STEPS; i++) {
    float inflow = 0.0;
    if (i < numInflow) {
      inflow = inflowHydrograph[i];
    }
    rModel->SetObsInflow(NUM_GRID_CELLS - 1, inflow);
    // currentFF[NUM_GRID_CELLS-1] = inflow * 3.6 / nodes[0].area;

    rModel->Route(stepHoursReal, &currentFF, &currentSF, &currentQ);
    totalIn += inflow;
    totalOut += currentQ[0];
    printf("time %f, inflow %f, outlet %f\n", i * 12.0, inflow, currentQ[0]);
    fprintf(file, "%f,%f,%f\n", i * 12.0, inflow, currentQ[0]);
  }
  fclose(file);
  printf("Total in %f, out %f\n", totalIn, totalOut);
}

void PrintStartupMessage() {
  printf("%s", "********************************************************\n");
  printf("%s", "**   Ensemble Framework For Flash Flood Forecasting   **\n");
  printf("**                   Version %s                     **\n",
         EF5_VERSION);
  printf("**                   KW Test                           **\n");
  printf("%s", "********************************************************\n");
}

#include "ExecutionController.h"
#include "ARS.h"
#include "BasicConfigSection.h"
#include "BasicGrids.h"
#include "BasinConfigSection.h"
#include "DREAM.h"
#include "EnsTaskConfigSection.h"
#include "ExecuteConfigSection.h"
#include "GaugeConfigSection.h"
#include "GeographicProjection.h"
#include "LAEAProjection.h"
#include "Messages.h"
#include "Model.h"
#include "Simulator.h"
#include "TaskConfigSection.h"
#include "TimeVar.h"
#include <cstdio>
#include <cstring>

static void LoadProjection();
static void ExecuteSimulation(TaskConfigSection *task);
static void ExecuteSimulationRP(TaskConfigSection *task);
static void ExecuteCalibrationARS(TaskConfigSection *task);
static void ExecuteCalibrationDREAM(TaskConfigSection *task);
static void ExecuteCalibrationDREAMEns(EnsTaskConfigSection *task);
static void ExecuteClipBasin(TaskConfigSection *task);
static void ExecuteClipGauge(TaskConfigSection *task);
static void ExecuteMakeBasic(TaskConfigSection *task);
static void ExecuteMakeBasinAvg(TaskConfigSection *task);

void ExecuteTasks() {

  if (!g_executeConfig) {
    ERROR_LOGF("%s", "No execute section specified!");
    return;
  }

  std::vector<TaskConfigSection *> *tasks = g_executeConfig->GetTasks();
  std::vector<TaskConfigSection *>::iterator taskItr;

  std::vector<EnsTaskConfigSection *> *ensTasks =
      g_executeConfig->GetEnsTasks();
  std::vector<EnsTaskConfigSection *>::iterator ensTaskItr;

  if (!LoadBasicGrids()) {
    return;
  }
  LoadProjection();

  // Loop through all the ensemble tasks and execute them first
  for (ensTaskItr = ensTasks->begin(); ensTaskItr != ensTasks->end();
       ensTaskItr++) {
    EnsTaskConfigSection *ensTask = (*ensTaskItr);
    INFO_LOGF("Executing ensemble task %s", ensTask->GetName());

    switch (ensTask->GetRunStyle()) {
    case STYLE_CALI_DREAM:
      ExecuteCalibrationDREAMEns(ensTask);
      break;
    default:
      ERROR_LOGF("Unsupport ensemble task run style \"%u\"",
                 ensTask->GetRunStyle());
      break;
    }
  }

  // Loop through all of the tasks and execute them
  for (taskItr = tasks->begin(); taskItr != tasks->end(); taskItr++) {
    TaskConfigSection *task = (*taskItr);
    INFO_LOGF("Executing task %s", task->GetName());

    switch (task->GetRunStyle()) {
    case STYLE_SIMU:
      ExecuteSimulation(task);
      break;
    case STYLE_SIMU_RP:
      ExecuteSimulationRP(task);
      break;
    case STYLE_CALI_ARS:
      ExecuteCalibrationARS(task);
      break;
    case STYLE_CALI_DREAM:
      ExecuteCalibrationDREAM(task);
      break;
    case STYLE_CLIP_BASIN:
      ExecuteClipBasin(task);
      break;
    case STYLE_CLIP_GAUGE:
      ExecuteClipGauge(task);
      break;
    case STYLE_MAKE_BASIC:
      ExecuteMakeBasic(task);
      break;
    case STYLE_BASIN_AVG:
      ExecuteMakeBasinAvg(task);
    default:
      ERROR_LOGF("Unimplemented simulation run style \"%u\"",
                 task->GetRunStyle());
      break;
    }
  }

  FreeBasicGridsData();
}

void LoadProjection() {

  switch (g_basicConfig->GetProjection()) {
  case PROJECTION_GEOGRAPHIC:
    g_Projection = new GeographicProjection();
    g_Projection->SetCellSize(g_DEM->cellSize);
    break;
  case PROJECTION_LAEA:
    g_Projection = new LAEAProjection();
    g_Projection->SetCellSize(g_DEM->cellSize);
    break;
  }

  // Reproject the gauges into the proper map coordinates
  for (std::map<std::string, GaugeConfigSection *>::iterator itr =
           g_gaugeConfigs.begin();
       itr != g_gaugeConfigs.end(); itr++) {

    GaugeConfigSection *gauge = itr->second;
    float newX, newY;
    g_Projection->ReprojectPoint(gauge->GetLon(), gauge->GetLat(), &newX,
                                 &newY);
    gauge->SetLon(newX);
    gauge->SetLat(newY);
  }
}

void ExecuteSimulation(TaskConfigSection *task) {

  Simulator sim;

  if (sim.Initialize(task)) {
    sim.Simulate();
    sim.CleanUp();
  }
}

void ExecuteSimulationRP(TaskConfigSection *task) {

  Simulator sim;

  sim.Initialize(task);
  sim.Simulate(true);
  sim.CleanUp();
}

void ExecuteCalibrationARS(TaskConfigSection *task) {

  Simulator sim;
  char buffer[CONFIG_MAX_LEN * 2];

  sim.Initialize(task);
  sprintf(buffer, "%s/%s", task->GetOutput(), "califorcings.bin");
  sim.PreloadForcings(buffer, true);

  printf("Precip loaded!\n");

  ARS ars;
  ars.Initialize(task->GetCaliParamSec(), task->GetRoutingCaliParamSec(), NULL,
                 numModelParams[task->GetModel()],
                 numRouteParams[task->GetRouting()], 0, &sim);
  ars.CalibrateParams();

  sprintf(buffer, "%s/cali_ars.%s.%s.csv", task->GetOutput(),
          task->GetCaliParamSec()->GetGauge()->GetName(),
          modelStrings[task->GetModel()]);
  ars.WriteOutput(buffer, task->GetModel(), task->GetRouting());
}

void ExecuteCalibrationDREAM(TaskConfigSection *task) {

  Simulator sim;
  char buffer[CONFIG_MAX_LEN * 2];

  sim.Initialize(task);
  sprintf(buffer, "%s/%s", task->GetOutput(), "califorcings.bin");
  sim.PreloadForcings(buffer, true);

  INFO_LOGF("%s", "Precip loaded!");

  DREAM dream;
  int numSnow = 0;
  if (task->GetSnow() != SNOW_QTY) {
    numSnow = numSnowParams[task->GetSnow()];
  }
  dream.Initialize(task->GetCaliParamSec(), task->GetRoutingCaliParamSec(),
                   task->GetSnowCaliParamSec(),
                   numModelParams[task->GetModel()],
                   numRouteParams[task->GetRouting()], numSnow, &sim);
  dream.CalibrateParams();

  sprintf(buffer, "%s/cali_dream.%s.%s.csv", task->GetOutput(),
          task->GetCaliParamSec()->GetGauge()->GetName(),
          modelStrings[task->GetModel()]);
  dream.WriteOutput(buffer, task->GetModel(), task->GetRouting(),
                    task->GetSnow());
}

void ExecuteCalibrationDREAMEns(EnsTaskConfigSection *task) {

  char buffer[CONFIG_MAX_LEN * 2];
  std::vector<TaskConfigSection *> *tasks = task->GetTasks();
  int numMembers = (int)tasks->size();
  int numParams = 0;

  std::vector<Simulator> sims;
  std::vector<int> paramsPerSim;
  paramsPerSim.resize(numMembers);
  sims.resize(numMembers);

  for (int i = 0; i < numMembers; i++) {
    Simulator *sim = &(sims[i]);
    TaskConfigSection *thisTask = tasks->at(i);
    sim->Initialize(thisTask);
    sprintf(buffer, "%s/%s", thisTask->GetOutput(), "califorcings.bin");
    sim->PreloadForcings(buffer, true);
    numParams += numModelParams[thisTask->GetModel()];
    paramsPerSim[i] = numModelParams[thisTask->GetModel()];
  }

  float *minParams = new float[numParams];
  float *maxParams = new float[numParams];
  int paramIndex = 0;

  for (int i = 0; i < numMembers; i++) {
    TaskConfigSection *thisTask = tasks->at(i);
    int cParams = numModelParams[thisTask->GetModel()];
    memcpy(&(minParams[paramIndex]),
           thisTask->GetCaliParamSec()->GetParamMins(),
           sizeof(float) * cParams);
    memcpy(&(maxParams[paramIndex]),
           thisTask->GetCaliParamSec()->GetParamMaxs(),
           sizeof(float) * cParams);
    paramIndex += cParams;
  }

  INFO_LOGF("%s", "Precip loaded!\n");

  DREAM dream;
  dream.Initialize(tasks->at(0)->GetCaliParamSec(), numParams, minParams,
                   maxParams, &sims, &paramsPerSim);
  dream.CalibrateParams();

  sprintf(buffer, "%s/cali_dream.%s.%s.csv", tasks->at(0)->GetOutput(),
          tasks->at(0)->GetCaliParamSec()->GetGauge()->GetName(), "ensemble");
  dream.WriteOutput(buffer, tasks->at(0)->GetModel(),
                    tasks->at(0)->GetRouting(), tasks->at(0)->GetSnow());
}

void ExecuteClipBasin(TaskConfigSection *task) {
  std::map<GaugeConfigSection *, float *> fullParamSettings, *paramSettings,
      fullRouteParamSettings, *routeParamSettings;
  std::vector<GridNode> nodes;
  GaugeMap gaugeMap;

  // Get the parameter settings for this task
  paramSettings = task->GetParamsSec()->GetParamSettings();
  float *defaultParams = NULL, *defaultRouteParams = NULL;
  GaugeConfigSection *gs = task->GetDefaultGauge();
  std::map<GaugeConfigSection *, float *>::iterator pitr =
      paramSettings->find(gs);
  if (pitr != paramSettings->end()) {
    defaultParams = pitr->second;
  }
  routeParamSettings = task->GetRoutingParamsSec()->GetParamSettings();
  pitr = routeParamSettings->find(gs);
  if (pitr != routeParamSettings->end()) {
    defaultRouteParams = pitr->second;
  }

  CarveBasin(task->GetBasinSec(), &nodes, paramSettings, &fullParamSettings,
             &gaugeMap, defaultParams, routeParamSettings,
             &fullRouteParamSettings, defaultRouteParams, NULL, NULL, NULL,
             NULL, NULL, NULL);

  ClipBasicGrids(task->GetBasinSec(), &nodes, task->GetBasinSec()->GetName(),
                 task->GetOutput());
}

void ExecuteClipGauge(TaskConfigSection *task) {
  std::map<GaugeConfigSection *, float *> fullParamSettings, *paramSettings,
      fullRouteParamSettings, *routeParamSettings;
  std::vector<GridNode> nodes;
  GaugeMap gaugeMap;

  // Get the parameter settings for this task
  paramSettings = task->GetParamsSec()->GetParamSettings();
  float *defaultParams = NULL, *defaultRouteParams = NULL;
  GaugeConfigSection *gs = task->GetDefaultGauge();
  std::map<GaugeConfigSection *, float *>::iterator pitr =
      paramSettings->find(gs);
  if (pitr != paramSettings->end()) {
    defaultParams = pitr->second;
  }
  routeParamSettings = task->GetRoutingParamsSec()->GetParamSettings();
  pitr = routeParamSettings->find(gs);
  if (pitr != routeParamSettings->end()) {
    defaultRouteParams = pitr->second;
  }

  CarveBasin(task->GetBasinSec(), &nodes, paramSettings, &fullParamSettings,
             &gaugeMap, defaultParams, routeParamSettings,
             &fullRouteParamSettings, defaultRouteParams, NULL, NULL, NULL,
             NULL, NULL, NULL);

  GridLoc *loc = (*(task->GetBasinSec()->GetGauges()))[0]->GetGridLoc();
  ClipBasicGrids(loc->x, loc->y, 10, task->GetOutput());
}

void ExecuteMakeBasic(TaskConfigSection *task) { MakeBasic(); }

void ExecuteMakeBasinAvg(TaskConfigSection *task) {
  Simulator sim;

  if (sim.Initialize(task)) {
    sim.BasinAvg();
    sim.CleanUp();
  }
}

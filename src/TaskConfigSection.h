#ifndef CONFIG_TASK_SECTION_H
#define CONFIG_TASK_SECTION_H

#include "BasinConfigSection.h"
#include "CaliParamConfigSection.h"
#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "InundationCaliParamConfigSection.h"
#include "InundationParamSetConfigSection.h"
#include "Model.h"
#include "PETConfigSection.h"
#include "ParamSetConfigSection.h"
#include "PrecipConfigSection.h"
#include "RoutingCaliParamConfigSection.h"
#include "RoutingParamSetConfigSection.h"
#include "SnowCaliParamConfigSection.h"
#include "SnowParamSetConfigSection.h"
#include "TempConfigSection.h"
#include "TimeUnit.h"
#include "TimeVar.h"
#include <map>

class TaskConfigSection : public ConfigSection {

public:
  TaskConfigSection(const char *nameVal);
  ~TaskConfigSection();

  char *GetName();
  char *GetOutput();
  char *GetState();
  char *GetStdGrid();
  char *GetAvgGrid();
  char *GetScGrid();
  char *GetActionGrid();
  char *GetMinorGrid();
  char *GetModerateGrid();
  char *GetMajorGrid();
  char *GetActionSDGrid();
  char *GetMinorSDGrid();
  char *GetModerateSDGrid();
  char *GetMajorSDGrid();
  char *GetPreloadForcings();
  char *GetDAFile();
  char *GetCOFile();
  TimeVar *GetTimeBegin();
  TimeVar *GetTimeWarmEnd();
  TimeVar *GetTimeEnd();
  TimeVar *GetTimeState();
  TimeUnit *GetTimeStep();
  TimeUnit *GetTimeStepLR();
  TimeVar *GetTimeBeginLR();
  PrecipConfigSection *GetPrecipSec();
  PrecipConfigSection *GetQPFSec();
  PETConfigSection *GetPETSec();
  TempConfigSection *GetTempSec();
  TempConfigSection *GetTempFSec();
  BasinConfigSection *GetBasinSec();
  ParamSetConfigSection *GetParamsSec();
  CaliParamConfigSection *GetCaliParamSec();
  RoutingParamSetConfigSection *GetRoutingParamsSec();
  RoutingCaliParamConfigSection *GetRoutingCaliParamSec();
  SnowParamSetConfigSection *GetSnowParamsSec();
  SnowCaliParamConfigSection *GetSnowCaliParamSec();
  InundationParamSetConfigSection *GetInundationParamsSec();
  InundationCaliParamConfigSection *GetInundationCaliParamSec();
  RUNSTYLE GetRunStyle();
  MODELS GetModel();
  ROUTES GetRouting();
  SNOWS GetSnow();
  INUNDATIONS GetInundation();
  GaugeConfigSection *GetDefaultGauge();
  bool UseStates() { return stateSet; }
  bool SaveStates() { return (stateSet && timeStateSet); }
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();
  int GetGriddedOutputs() { return griddedOutputs; }

  static bool IsDuplicate(char *name);

private:
  bool styleSet, modelSet, basinSet, precipSet, qpfSet, petSet, outputSet,
      stateSet, paramsSet, timestepSet, timeStateSet, timeBeginSet,
      timeWarmEndSet, timeEndSet, caliParamSet, routingParamsSet,
      routingCaliParamSet, defaultParamsSet, routeSet;
  bool snowParamsSet, snowCaliParamSet, snowSet, tempSet, tempFSet;
  bool inundationParamsSet, inundationCaliParamSet, inundationSet;
  bool timeBeginLRSet, timestepLRSet;
  char output[CONFIG_MAX_LEN], state[CONFIG_MAX_LEN];
  char name[CONFIG_MAX_LEN];
  char stdGrid[CONFIG_MAX_LEN], avgGrid[CONFIG_MAX_LEN], scGrid[CONFIG_MAX_LEN];
  char actionGrid[CONFIG_MAX_LEN], minorGrid[CONFIG_MAX_LEN],
      moderateGrid[CONFIG_MAX_LEN], majorGrid[CONFIG_MAX_LEN];
  char actionSDGrid[CONFIG_MAX_LEN], minorSDGrid[CONFIG_MAX_LEN],
      moderateSDGrid[CONFIG_MAX_LEN], majorSDGrid[CONFIG_MAX_LEN];
  char preloadFile[CONFIG_MAX_LEN];
  char daFile[CONFIG_MAX_LEN];
  char coFile[CONFIG_MAX_LEN];
  MODELS model;
  ROUTES routing;
  SNOWS snow;
  INUNDATIONS inundation;
  BasinConfigSection *basin;
  PrecipConfigSection *precip, *qpf;
  PETConfigSection *pet;
  TempConfigSection *temp, *tempf;
  ParamSetConfigSection *params;
  CaliParamConfigSection *caliParam;
  RoutingParamSetConfigSection *paramsRouting;
  RoutingCaliParamConfigSection *caliParamRouting;
  SnowParamSetConfigSection *paramsSnow;
  SnowCaliParamConfigSection *caliParamSnow;
  InundationParamSetConfigSection *paramsInundation;
  InundationCaliParamConfigSection *caliParamInundation;
  GaugeConfigSection *defaultGauge;
  TimeUnit timeStep;
  TimeUnit timeStepLR;
  RUNSTYLE style;
  TimeVar timeBegin;
  TimeVar timeWarmEnd;
  TimeVar timeEnd;
  TimeVar timeState;
  TimeVar timeBeginLR;
  int griddedOutputs;

  bool LoadGriddedOutputs(char *value);
};

extern std::map<std::string, TaskConfigSection *> g_taskConfigs;

#endif

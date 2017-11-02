#include "TaskConfigSection.h"
#include "GriddedOutput.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::map<std::string, TaskConfigSection *> g_taskConfigs;

TaskConfigSection::TaskConfigSection(const char *nameVal) {
  strcpy(name, nameVal);
  styleSet = false;
  modelSet = false;
  routeSet = false;
  snowSet = false;
  inundationSet = false;
  basinSet = false;
  precipSet = false;
  qpfSet = false;
  petSet = false;
  tempSet = false;
  tempFSet = false;
  outputSet = false;
  paramsSet = false;
  routingParamsSet = false;
  snowParamsSet = false;
  inundationParamsSet = false;

  timestepSet = false;
  timestepLRSet = false;
  timeStateSet = false;
  timeBeginSet = false;
  timeBeginLRSet = false;
  timeWarmEndSet = false;
  timeEndSet = false;
  caliParamSet = false;
  snowCaliParamSet = false;
  inundationCaliParamSet = false;
  routingCaliParamSet = false;
  defaultParamsSet = false;
  defaultGauge = NULL;
  qpf = NULL;
  tempf = NULL;
  stdGrid[0] = 0;
  avgGrid[0] = 0;
  scGrid[0] = 0;
  actionGrid[0] = 0;
  minorGrid[0] = 0;
  moderateGrid[0] = 0;
  majorGrid[0] = 0;
  actionSDGrid[0] = 0;
  minorSDGrid[0] = 0;
  moderateSDGrid[0] = 0;
  majorSDGrid[0] = 0;
  memset(daFile, 0, CONFIG_MAX_LEN);
  memset(preloadFile, 0, CONFIG_MAX_LEN);
  memset(coFile, 0, CONFIG_MAX_LEN);
  griddedOutputs = OG_NONE;
  routing = ROUTE_QTY;
  snow = SNOW_QTY;
  inundation = INUNDATION_QTY;
  temp = NULL;
}

TaskConfigSection::~TaskConfigSection() {}

char *TaskConfigSection::GetName() { return name; }

char *TaskConfigSection::GetOutput() { return output; }

char *TaskConfigSection::GetState() { return state; }

char *TaskConfigSection::GetStdGrid() { return stdGrid; }

char *TaskConfigSection::GetAvgGrid() { return avgGrid; }

char *TaskConfigSection::GetScGrid() { return scGrid; }

char *TaskConfigSection::GetActionGrid() { return actionGrid; }

char *TaskConfigSection::GetMinorGrid() { return minorGrid; }

char *TaskConfigSection::GetModerateGrid() { return moderateGrid; }

char *TaskConfigSection::GetMajorGrid() { return majorGrid; }

char *TaskConfigSection::GetActionSDGrid() { return actionSDGrid; }

char *TaskConfigSection::GetMinorSDGrid() { return minorSDGrid; }

char *TaskConfigSection::GetModerateSDGrid() { return moderateSDGrid; }

char *TaskConfigSection::GetMajorSDGrid() { return majorSDGrid; }

char *TaskConfigSection::GetPreloadForcings() { return preloadFile; }

char *TaskConfigSection::GetDAFile() { return daFile; }

char *TaskConfigSection::GetCOFile() { return coFile; }

TimeVar *TaskConfigSection::GetTimeBegin() { return &timeBegin; }

TimeVar *TaskConfigSection::GetTimeState() { return &timeState; }

TimeVar *TaskConfigSection::GetTimeWarmEnd() { return &timeWarmEnd; }

TimeVar *TaskConfigSection::GetTimeEnd() { return &timeEnd; }

TimeVar *TaskConfigSection::GetTimeBeginLR() {
  if (!timestepLRSet || !timeBeginLRSet) {
    return NULL;
  }
  return &timeBeginLR;
}

TimeUnit *TaskConfigSection::GetTimeStep() { return &timeStep; }

TimeUnit *TaskConfigSection::GetTimeStepLR() {
  if (!timestepLRSet || !timeBeginLRSet) {
    return NULL;
  }
  return &timeStepLR;
}

PrecipConfigSection *TaskConfigSection::GetPrecipSec() { return precip; }

PrecipConfigSection *TaskConfigSection::GetQPFSec() { return qpf; }

PETConfigSection *TaskConfigSection::GetPETSec() { return pet; }

TempConfigSection *TaskConfigSection::GetTempSec() { return temp; }

TempConfigSection *TaskConfigSection::GetTempFSec() { return tempf; }

BasinConfigSection *TaskConfigSection::GetBasinSec() { return basin; }

ParamSetConfigSection *TaskConfigSection::GetParamsSec() { return params; }

CaliParamConfigSection *TaskConfigSection::GetCaliParamSec() {
  return caliParam;
}

RoutingParamSetConfigSection *TaskConfigSection::GetRoutingParamsSec() {
  return paramsRouting;
}

RoutingCaliParamConfigSection *TaskConfigSection::GetRoutingCaliParamSec() {
  return caliParamRouting;
}

SnowParamSetConfigSection *TaskConfigSection::GetSnowParamsSec() {
  return paramsSnow;
}

SnowCaliParamConfigSection *TaskConfigSection::GetSnowCaliParamSec() {
  return caliParamSnow;
}

InundationParamSetConfigSection *TaskConfigSection::GetInundationParamsSec() {
  return paramsInundation;
}

InundationCaliParamConfigSection *
TaskConfigSection::GetInundationCaliParamSec() {
  return caliParamInundation;
}

RUNSTYLE TaskConfigSection::GetRunStyle() { return style; }

MODELS TaskConfigSection::GetModel() { return model; }

ROUTES TaskConfigSection::GetRouting() { return routing; }

SNOWS TaskConfigSection::GetSnow() { return snow; }

INUNDATIONS TaskConfigSection::GetInundation() { return inundation; }

GaugeConfigSection *TaskConfigSection::GetDefaultGauge() {
  return defaultGauge;
}

CONFIG_SEC_RET TaskConfigSection::ProcessKeyValue(char *name, char *value) {

  if (!strcasecmp(name, "style")) {
    for (int i = 0; i < STYLE_QTY; i++) {
      if (!strcasecmp(value, runStyleStrings[i])) {
        styleSet = true;
        style = (RUNSTYLE)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown run style option \"%s\"!", value);
    INFO_LOGF("Valid run style options are \"%s\"", "SIMU, SIMU_RP, CALI_ARS, "
                                                    "CALI_DREAM, CLIP_BASIN, "
                                                    "CLIP_GAUGE, BASIN_AVG");
    return INVALID_RESULT;
  } else if (!strcasecmp(name, "model")) {
    for (int i = 0; i < MODEL_QTY; i++) {
      if (!strcasecmp(value, modelStrings[i])) {
        modelSet = true;
        model = (MODELS)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown water balance model option \"%s\"!", value);
    INFO_LOGF("Valid water balance options are \"%s\"", "CREST, SAC, HP");
    return INVALID_RESULT;
  } else if (!strcasecmp(name, "routing")) {
    for (int i = 0; i < ROUTE_QTY; i++) {
      if (!strcasecmp(value, routeStrings[i])) {
        routeSet = true;
        routing = (ROUTES)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown routing option \"%s\"!", value);
    INFO_LOGF("Valid routing options are \"%s\"", "LR, KW");
    return INVALID_RESULT;
  } else if (!strcasecmp(name, "snow")) {
    for (int i = 0; i < SNOW_QTY; i++) {
      if (!strcasecmp(value, snowStrings[i])) {
        snowSet = true;
        snow = (SNOWS)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown snow option \"%s\"!", value);
    INFO_LOGF("Valid snow options are \"%s\"", "SNOW17");
    return INVALID_RESULT;
  } else if (!strcasecmp(name, "inundation")) {
    for (int i = 0; i < INUNDATION_QTY; i++) {
      if (!strcasecmp(value, inundationStrings[i])) {
        inundationSet = true;
        inundation = (INUNDATIONS)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown inundation option \"%s\"!", value);
    INFO_LOGF("Valid inundation options are \"%s\"",
              "SIMPLEINUNDATION, VCINUNDATION");
    return INVALID_RESULT;
  } else if (!strcasecmp(name, "basin")) {
    TOLOWER(value);
    std::map<std::string, BasinConfigSection *>::iterator itr =
        g_basinConfigs.find(value);
    if (itr == g_basinConfigs.end()) {
      ERROR_LOGF("Unknown basin \"%s\"!", value);
      return INVALID_RESULT;
    }
    basin = itr->second;
    basinSet = true;
  } else if (!strcasecmp(name, "defaultparamsgauge")) {
    TOLOWER(value);
    std::map<std::string, GaugeConfigSection *>::iterator itr =
        g_gaugeConfigs.find(value);
    if (itr == g_gaugeConfigs.end()) {
      ERROR_LOGF("Unknown default parameter gauge \"%s\"!", value);
      return INVALID_RESULT;
    }
    defaultGauge = itr->second;
    defaultParamsSet = true;
  } else if (!strcasecmp(name, "precip")) {
    TOLOWER(value);
    std::map<std::string, PrecipConfigSection *>::iterator itr =
        g_precipConfigs.find(value);
    if (itr == g_precipConfigs.end()) {
      ERROR_LOGF("Unknown precip setting \"%s\"!", value);
      return INVALID_RESULT;
    }
    precip = itr->second;
    precipSet = true;
  } else if (!strcasecmp(name, "precipforecast")) {
    TOLOWER(value);
    std::map<std::string, PrecipConfigSection *>::iterator itr =
        g_precipConfigs.find(value);
    if (itr == g_precipConfigs.end()) {
      ERROR_LOGF("Unknown precip forecast setting \"%s\"!", value);
      return INVALID_RESULT;
    }
    qpf = itr->second;
    qpfSet = true;
  } else if (!strcasecmp(name, "pet")) {
    TOLOWER(value);
    std::map<std::string, PETConfigSection *>::iterator itr =
        g_petConfigs.find(value);
    if (itr == g_petConfigs.end()) {
      ERROR_LOGF("Unknown PET setting \"%s\"!", value);
      return INVALID_RESULT;
    }
    pet = itr->second;
    petSet = true;
  } else if (!strcasecmp(name, "temp")) {
    TOLOWER(value);
    std::map<std::string, TempConfigSection *>::iterator itr =
        g_tempConfigs.find(value);
    if (itr == g_tempConfigs.end()) {
      ERROR_LOGF("Unknown temp setting \"%s\"!", value);
      return INVALID_RESULT;
    }
    temp = itr->second;
    tempSet = true;
  } else if (!strcasecmp(name, "tempforecast")) {
    TOLOWER(value);
    std::map<std::string, TempConfigSection *>::iterator itr =
        g_tempConfigs.find(value);
    if (itr == g_tempConfigs.end()) {
      ERROR_LOGF("Unknown temp forecast setting \"%s\"!", value);
      return INVALID_RESULT;
    }
    tempf = itr->second;
    tempFSet = true;
  } else if (!strcasecmp(name, "param_set")) {
    if (!modelSet) {
      ERROR_LOG("The MODEL setting must be specified before the PARAM_SET "
                "setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, ParamSetConfigSection *>::iterator itr =
        g_paramSetConfigs[model].find(value);
    if (itr == g_paramSetConfigs[model].end()) {
      ERROR_LOGF("Unknown parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    params = itr->second;
    paramsSet = true;
  } else if (!strcasecmp(name, "routing_param_set")) {
    if (!routeSet) {
      ERROR_LOG("The ROUTING setting must be specified before the "
                "ROUTING_PARAM_SET setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, RoutingParamSetConfigSection *>::iterator itr =
        g_routingParamSetConfigs[routing].find(value);
    if (itr == g_routingParamSetConfigs[routing].end()) {
      ERROR_LOGF("Unknown routing parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    paramsRouting = itr->second;
    routingParamsSet = true;
  } else if (!strcasecmp(name, "snow_param_set")) {
    if (!snowSet) {
      ERROR_LOG("The SNOW setting must be specified before the SNOW_PARAM_SET "
                "setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, SnowParamSetConfigSection *>::iterator itr =
        g_snowParamSetConfigs[snow].find(value);
    if (itr == g_snowParamSetConfigs[snow].end()) {
      ERROR_LOGF("Unknown snow parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    paramsSnow = itr->second;
    snowParamsSet = true;
  } else if (!strcasecmp(name, "inundation_param_set")) {
    if (!inundationSet) {
      ERROR_LOG("The INUNDATION setting must be specified before the "
                "INUNDATION_PARAM_SET setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, InundationParamSetConfigSection *>::iterator itr =
        g_inundationParamSetConfigs[inundation].find(value);
    if (itr == g_inundationParamSetConfigs[inundation].end()) {
      ERROR_LOGF("Unknown inundation parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    paramsInundation = itr->second;
    inundationParamsSet = true;
  } else if (!strcasecmp(name, "output")) {
    strcpy(output, value);
    outputSet = true;
  } else if (!strcasecmp(name, "preload_file")) {
    strcpy(preloadFile, value);
  } else if (!strcasecmp(name, "da_file")) {
    strcpy(daFile, value);
  } else if (!strcasecmp(name, "co_file")) {
    strcpy(coFile, value);
  } else if (!strcasecmp(name, "states")) {
    strcpy(state, value);
    stateSet = true;
  } else if (!strcasecmp(name, "output_grids")) {
    if (!LoadGriddedOutputs(value)) {
      return INVALID_RESULT;
    }
  } else if (!strcasecmp(name, "rp_stdgrid")) {
    strcpy(stdGrid, value);
  } else if (!strcasecmp(name, "rp_avggrid")) {
    strcpy(avgGrid, value);
  } else if (!strcasecmp(name, "rp_csgrid")) {
    strcpy(scGrid, value);
  } else if (!strcasecmp(name, "action_grid")) {
    strcpy(actionGrid, value);
  } else if (!strcasecmp(name, "minor_grid")) {
    strcpy(minorGrid, value);
  } else if (!strcasecmp(name, "moderate_grid")) {
    strcpy(moderateGrid, value);
  } else if (!strcasecmp(name, "major_grid")) {
    strcpy(majorGrid, value);
  } else if (!strcasecmp(name, "action_sd_grid")) {
    strcpy(actionSDGrid, value);
  } else if (!strcasecmp(name, "minor_sd_grid")) {
    strcpy(minorSDGrid, value);
  } else if (!strcasecmp(name, "moderate_sd_grid")) {
    strcpy(moderateSDGrid, value);
  } else if (!strcasecmp(name, "major_sd_grid")) {
    strcpy(majorSDGrid, value);
  } else if (!strcasecmp(name, "timestep")) {
    SUPPORTED_TIME_UNITS result = timeStep.ParseUnit(value);
    if (result == TIME_UNIT_QTY) {
      ERROR_LOGF("Unknown timestep option \"%s\"", value);
      return INVALID_RESULT;
    }
    timestepSet = true;
  } else if (!strcasecmp(name, "timestep_lr")) {
    SUPPORTED_TIME_UNITS result = timeStepLR.ParseUnit(value);
    if (result == TIME_UNIT_QTY) {
      ERROR_LOGF("Unknown timestep long range option \"%s\"", value);
      return INVALID_RESULT;
    }
    timestepLRSet = true;
  } else if (!strcasecmp(name, "time_state")) {
    if (!timeState.LoadTime(value)) {
      ERROR_LOGF("Unknown time state option \"%s\"", value);
      return INVALID_RESULT;
    }
    timeStateSet = true;
  } else if (!strcasecmp(name, "time_begin")) {
    if (!timeBegin.LoadTime(value)) {
      ERROR_LOGF("Unknown time begin option \"%s\"", value);
      return INVALID_RESULT;
    }
    timeBeginSet = true;
  } else if (!strcasecmp(name, "time_begin_lr")) {
    if (!timeBeginLR.LoadTime(value)) {
      ERROR_LOGF("Unknown time begin long range option \"%s\"", value);
      return INVALID_RESULT;
    }
    timeBeginLRSet = true;
  } else if (!strcasecmp(name, "time_warmend")) {
    if (!timeWarmEnd.LoadTime(value)) {
      ERROR_LOGF("Unknown time warm end option \"%s\"", value);
      return INVALID_RESULT;
    }
    timeWarmEndSet = true;
  } else if (!strcasecmp(name, "time_end")) {
    if (!timeEnd.LoadTime(value)) {
      ERROR_LOGF("Unknown time end option \"%s\"", value);
      return INVALID_RESULT;
    }
    timeEndSet = true;
  } else if (!strcasecmp(name, "cali_param")) {
    if (!modelSet) {
      ERROR_LOG("The MODEL setting must be specified before the CALI_PARAM "
                "setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, CaliParamConfigSection *>::iterator itr =
        g_caliParamConfigs[model].find(value);
    if (itr == g_caliParamConfigs[model].end()) {
      ERROR_LOGF("Unknown calibration parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    caliParam = itr->second;
    caliParamSet = true;
  } else if (!strcasecmp(name, "routing_cali_param")) {
    if (!routeSet) {
      ERROR_LOG("The ROUTING setting must be specified before the "
                "ROUTING_CALI_PARAM setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, RoutingCaliParamConfigSection *>::iterator itr =
        g_routingCaliParamConfigs[routing].find(value);
    if (itr == g_routingCaliParamConfigs[routing].end()) {
      ERROR_LOGF("Unknown routing calibration parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    caliParamRouting = itr->second;
    routingCaliParamSet = true;
  } else if (!strcasecmp(name, "snow_cali_param")) {
    if (!snowSet) {
      ERROR_LOG("The SNOW setting must be specified before the SNOW_CALI_PARAM "
                "setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, SnowCaliParamConfigSection *>::iterator itr =
        g_snowCaliParamConfigs[snow].find(value);
    if (itr == g_snowCaliParamConfigs[snow].end()) {
      ERROR_LOGF("Unknown snow calibration parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    caliParamSnow = itr->second;
    snowCaliParamSet = true;
  } else if (!strcasecmp(name, "inundation_cali_param")) {
    if (!inundationSet) {
      ERROR_LOG("The INUNDATION setting must be specified before the "
                "INUNDATION_CALI_PARAM setting in the task!");
      return INVALID_RESULT;
    }
    TOLOWER(value);
    std::map<std::string, InundationCaliParamConfigSection *>::iterator itr =
        g_inundationCaliParamConfigs[inundation].find(value);
    if (itr == g_inundationCaliParamConfigs[inundation].end()) {
      ERROR_LOGF("Unknown inundation calibration parameter set \"%s\"!", value);
      return INVALID_RESULT;
    }
    caliParamInundation = itr->second;
    inundationCaliParamSet = true;
  } else {
    ERROR_LOGF("Unknown task configuration option \"%s\"", name);
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

bool TaskConfigSection::LoadGriddedOutputs(char *value) {

  char *part = strtok(value, "|");
  while (part != NULL) {
    bool matchFound = false;
    for (int i = 0; i < OG_QTY; i++) {
      if (!strcasecmp(part, GriddedOutputText[i])) {
        griddedOutputs |= GriddedOutputFlags[i];
        matchFound = true;
        break;
      }
    }
    if (!matchFound) {
      ERROR_LOGF("Unknown output grids flag \"%s\"", part);
      return false;
    }
    part = strtok(NULL, "|");
  }

  return true;
}

CONFIG_SEC_RET TaskConfigSection::ValidateSection() {
  if (!styleSet) {
    ERROR_LOG("The run style was not specified");
    return INVALID_RESULT;
  } else if (!modelSet) {
    ERROR_LOG("The water balance model was not specified");
    return INVALID_RESULT;
  } else if (!basinSet) {
    ERROR_LOG("The basin was not specified");
    return INVALID_RESULT;
  } else if (!precipSet) {
    ERROR_LOG("The precip was not specified");
    return INVALID_RESULT;
  } else if (!petSet) {
    ERROR_LOG("The pet was not specified");
    return INVALID_RESULT;
  } else if (!outputSet) {
    ERROR_LOG("The output directory was not specified");
    return INVALID_RESULT;
  } else if (!paramsSet) {
    ERROR_LOG("The water balance parameter set was not specified");
    return INVALID_RESULT;
  } else if (routeSet && !routingParamsSet) {
    ERROR_LOG("The routing parameter set was not specified");
    return INVALID_RESULT;
  } else if (!timestepSet) {
    ERROR_LOG("The timestep was not specified");
    return INVALID_RESULT;
  } else if (!timeBeginSet) {
    ERROR_LOG("The beginning time was not specified");
    return INVALID_RESULT;
  } else if (!timeEndSet) {
    ERROR_LOG("The ending time was not specified");
    return INVALID_RESULT;
  } else if (snowSet && !snowParamsSet) {
    ERROR_LOG("The snow parameter set was not specified");
    return INVALID_RESULT;
  } else if (snowSet && !tempSet) {
    ERROR_LOG("The temperature forcing was not specified");
    return INVALID_RESULT;
  } else if (inundationSet && !inundationParamsSet) {
    ERROR_LOG("The inundation parameter set was not specified");
    return INVALID_RESULT;
  }

  if (IsCalibrationRunStyle(style)) {
    if (!caliParamSet) {
      ERROR_LOG(
          "The calibration water balance parameter set was not specified");
      return INVALID_RESULT;
    }

    if (!routingCaliParamSet) {
      ERROR_LOG("The calibration routing parameter set was not specified");
      return INVALID_RESULT;
    }

    if (snowSet && !snowCaliParamSet) {
      ERROR_LOG("The calibration snow parameter set was not specified");
      return INVALID_RESULT;
    }

    char *gaugeWB = caliParam->GetGauge()->GetName();
    char *gaugeR = caliParamRouting->GetGauge()->GetName();
    char *gaugeS = NULL;
    if (snowSet) {
      gaugeS = caliParamSnow->GetGauge()->GetName();
    }
    bool foundWB = false, foundR = false, foundS = false;
    std::vector<GaugeConfigSection *> *bGauges = basin->GetGauges();
    for (size_t i = 0; i < bGauges->size(); i++) {
      if (!strcasecmp(gaugeWB, bGauges->at(i)->GetName())) {
        foundWB = true;
      }
      if (!strcasecmp(gaugeR, bGauges->at(i)->GetName())) {
        foundR = true;
      }
      if (snowSet && !strcasecmp(gaugeS, bGauges->at(i)->GetName())) {
        foundS = true;
      }
    }

    if (!foundWB) {
      ERROR_LOG("The calibration gauge (water balance cali param) was not "
                "found in the basin!");
      return INVALID_RESULT;
    }
    if (!foundR) {
      ERROR_LOG("The calibration gauge (routing cali param) was not found in "
                "the basin!");
      return INVALID_RESULT;
    }
    if (snowSet && !foundS) {
      ERROR_LOG("The calibration gauge (snow cali param) was not found in the "
                "basin!");
      return INVALID_RESULT;
    }
  }

  if (!timeWarmEndSet) {
    timeWarmEnd = timeBegin;
  }

  return VALID_RESULT;
}

bool TaskConfigSection::IsDuplicate(char *name) {
  std::map<std::string, TaskConfigSection *>::iterator itr =
      g_taskConfigs.find(name);
  if (itr == g_taskConfigs.end()) {
    return false;
  } else {
    return true;
  }
}

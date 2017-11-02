#include "GaugeConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

std::map<std::string, GaugeConfigSection *> g_gaugeConfigs;

GaugeConfigSection::GaugeConfigSection(char *nameVal) {
  obsSet = false;
  latSet = false;
  lonSet = false;
  xSet = false;
  ySet = false;
  obsFlowAccumSet = false;
  strcpy(name, nameVal);
  outputTS = true;
  observation[0] = 0;
  wantDA = true;
  wantCO = false;
  continueUpstream = true;
}

GaugeConfigSection::~GaugeConfigSection() {}

char *GaugeConfigSection::GetName() { return name; }

void GaugeConfigSection::LoadTS() {
  if (observation[0]) {
    obs.LoadTimeSeries(observation);
  }
}

float GaugeConfigSection::GetObserved(TimeVar *currentTime) {
  if (!obs.GetNumberOfObs()) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  return obs.GetValueAtTime(currentTime);
}

float GaugeConfigSection::GetObserved(TimeVar *currentTime, float diff) {
  if (!obs.GetNumberOfObs()) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  return obs.GetValueNearTime(currentTime, diff);
}

void GaugeConfigSection::SetObservedValue(char *timeBuffer, float dataValue) {
  obs.PutValueAtTime(timeBuffer, dataValue);
}

CONFIG_SEC_RET GaugeConfigSection::ProcessKeyValue(char *name, char *value) {

  if (!strcasecmp(name, "lat")) {
    lat = strtod(value, NULL);
    latSet = true;
  } else if (!strcasecmp(name, "lon")) {
    lon = strtod(value, NULL);
    lonSet = true;
  } else if (!strcasecmp(name, "cellx")) {
    SetCellX(atoi(value));
    xSet = true;
  } else if (!strcasecmp(name, "celly")) {
    SetCellY(atoi(value));
    ySet = true;
  } else if (!strcasecmp(name, "basinarea")) {
    obsFlowAccum = atof(value);
    obsFlowAccumSet = true;
  } else if (!strcasecmp(name, "obs")) {
    strcpy(observation, value);
    obsSet = true;
  } else if (!strcasecmp(name, "outputts")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      outputTS = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      outputTS = true;
    } else {
      ERROR_LOGF("Unknown OUTPUTTS option \"%s\"", value);
      INFO_LOGF("Valid OUTPUTTS options are \"%s\"", "TRUE, FALSE");
      return INVALID_RESULT;
    }
    outputTSSet = true;
  } else if (!strcasecmp(name, "wantda")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      wantDA = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      wantDA = true;
    } else {
      ERROR_LOGF("Unknown WANTDA option \"%s\"", value);
      INFO_LOGF("Valid WANTDA options are \"%s\"", "TRUE, FALSE");
      return INVALID_RESULT;
    }
  } else if (!strcasecmp(name, "wantco")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      wantCO = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      wantCO = true;
    } else {
      ERROR_LOGF("Unknown WANTCO option \"%s\"", value);
      INFO_LOGF("Valid WANTCO options are \"%s\"", "TRUE, FALSE");
      return INVALID_RESULT;
    }
  } else if (!strcasecmp(name, "continueupstream")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      continueUpstream = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      continueUpstream = true;
    } else {
      ERROR_LOGF("Unknown CONTINUEUPSTREAM option \"%s\"", value);
      INFO_LOGF("Valid CONTINUEUPSTREAM options are \"%s\"", "TRUE, FALSE");
      return INVALID_RESULT;
    }
  } else {
    ERROR_LOGF("Unknown key value \"%s=%s\" in gauge %s!", name, value,
               this->name);
    return INVALID_RESULT;
  }
  return VALID_RESULT;
}

CONFIG_SEC_RET GaugeConfigSection::ValidateSection() {
  if (!latSet && !ySet) {
    ERROR_LOG("The latitude was not specified");
    return INVALID_RESULT;
  } else if (!lonSet && !xSet) {
    ERROR_LOG("The longitude was not specified");
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

bool GaugeConfigSection::IsDuplicate(char *name) {
  std::map<std::string, GaugeConfigSection *>::iterator itr =
      g_gaugeConfigs.find(name);
  if (itr == g_gaugeConfigs.end()) {
    return false;
  } else {
    return true;
  }
}

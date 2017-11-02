#include "DamConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

std::map<std::string, DamConfigSection *> g_damConfigs;

DamConfigSection::DamConfigSection(char *nameVal) {
  obsSet = false;
  latSet = false;
  lonSet = false;
  xSet = false;
  ySet = false;
  obsFlowAccumSet = false;
  volumeSet = false;
  strcpy(name, nameVal);
  outputTS = true;
  observation[0] = 0;
  wantDA = true;
  wantCO = false;
}

DamConfigSection::~DamConfigSection() {}

char *DamConfigSection::GetName() { return name; }

void DamConfigSection::LoadTS() {
  if (observation[0]) {
    obs.LoadTimeSeries(observation);
  }
}

float DamConfigSection::GetObserved(TimeVar *currentTime) {
  if (!obs.GetNumberOfObs()) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  return obs.GetValueAtTime(currentTime);
}

float DamConfigSection::GetObserved(TimeVar *currentTime, float diff) {
  if (!obs.GetNumberOfObs()) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  return obs.GetValueNearTime(currentTime, diff);
}

void DamConfigSection::SetObservedValue(char *timeBuffer, float dataValue) {
  obs.PutValueAtTime(timeBuffer, dataValue);
}

CONFIG_SEC_RET DamConfigSection::ProcessKeyValue(char *name, char *value) {

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
  } else if (!strcasecmp(name, "volume")) {
    volume = atof(value);
    volumeSet = true;
  } else if (!strcasecmp(name, "obs")) {
    strcpy(observation, value);
    obsSet = true;
  } else if (!strcasecmp(name, "outputts")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      outputTS = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      outputTS = true;
    } else {
      // Unknown value
    }
    outputTSSet = true;
  } else if (!strcasecmp(name, "wantda")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      wantDA = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      wantDA = true;
    } else {
      // Unknown value
    }
  } else if (!strcasecmp(name, "wantco")) {
    if (!strcasecmp(value, "false") || !strcasecmp(value, "no")) {
      wantCO = false;
    } else if (!strcasecmp(value, "true") || !strcasecmp(value, "yes")) {
      wantCO = true;
    } else {
      // Unknown value
    }
  }
  return VALID_RESULT;
}

CONFIG_SEC_RET DamConfigSection::ValidateSection() {
  if (!latSet && !ySet) {
    ERROR_LOG("The latitude was not specified");
    return INVALID_RESULT;
  } else if (!lonSet && !xSet) {
    ERROR_LOG("The longitude was not specified");
    return INVALID_RESULT;
  } else if (!volumeSet) {
    ERROR_LOG("Missing dam volume");
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

bool DamConfigSection::IsDuplicate(char *name) {
  std::map<std::string, DamConfigSection *>::iterator itr =
      g_damConfigs.find(name);
  if (itr == g_damConfigs.end()) {
    return false;
  } else {
    return true;
  }
}

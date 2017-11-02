#include "SnowCaliParamConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::map<std::string, SnowCaliParamConfigSection *>
    g_snowCaliParamConfigs[SNOW_QTY];

SnowCaliParamConfigSection::SnowCaliParamConfigSection(char *nameVal,
                                                       SNOWS snowVal) {
  strcpy(name, nameVal);
  gauge = NULL;
  snow = snowVal;
  int numParams = numSnowParams[snow];
  modelParamMins = new float[numParams];
  modelParamMaxs = new float[numParams];
  modelParamInits = new float[numParams];
  paramsSet = new bool[numParams];
  memset(modelParamMins, 0, sizeof(float) * numParams);
  memset(modelParamMins, 0, sizeof(float) * numParams);
  memset(modelParamInits, 0, sizeof(float) * numParams);
  memset(paramsSet, 0, sizeof(bool) * numParams);
}

SnowCaliParamConfigSection::~SnowCaliParamConfigSection() {
  delete[] modelParamMins;
  delete[] modelParamMaxs;
  delete[] modelParamInits;
}

char *SnowCaliParamConfigSection::GetName() { return name; }

CONFIG_SEC_RET SnowCaliParamConfigSection::ProcessKeyValue(char *name,
                                                           char *value) {
  int numParams = numSnowParams[snow];

  if (!strcasecmp(name, "gauge")) {

    TOLOWER(value);
    std::map<std::string, GaugeConfigSection *>::iterator itr =
        g_gaugeConfigs.find(value);
    if (itr == g_gaugeConfigs.end()) {
      ERROR_LOGF("Unknown gauge \"%s\" in parameter set!", value);
      return INVALID_RESULT;
    }

    gauge = itr->second;
  } else {

    if (!gauge) {
      ERROR_LOGF("Got parameter %s without a gauge being set!", name);
      return INVALID_RESULT;
    }

    // Lets see if this belongs to a parameter
    for (int i = 0; i < numParams; i++) {
      if (strcasecmp(name, snowParamStrings[snow][i]) == 0) {

        if (paramsSet[i]) {
          ERROR_LOGF("Duplicate parameter \"%s\" in parameter set!", name);
          return INVALID_RESULT;
        }

        float minVal = 0, maxVal = 0, initVal = 0;
        int count = sscanf(value, "%f,%f,%f", &minVal, &maxVal, &initVal);

        if (count < 2) {
          ERROR_LOGF("Parameter \"%s\" has invalid calibration values!", name);
          return INVALID_RESULT;
        }

        modelParamMins[i] = minVal;
        modelParamMaxs[i] = maxVal;
        modelParamInits[i] = initVal;
        paramsSet[i] = true;

        return VALID_RESULT;
      }
    }

    // We got here so we must not know what this parameter is!
    ERROR_LOGF("Unknown parameter name \"%s\".", name);
    return INVALID_RESULT;
  }
  return VALID_RESULT;
}

CONFIG_SEC_RET SnowCaliParamConfigSection::ValidateSection() {
  int numParams = numSnowParams[snow];

  if (!gauge) {
    ERROR_LOG("The gauge on which calibration is to be performed was not set!");
    return INVALID_RESULT;
  }

  for (int i = 0; i < numParams; i++) {
    if (!paramsSet[i]) {
      ERROR_LOGF(
          "Incomplete parameter set! Parameter \"%s\" was not given a value.",
          snowParamStrings[snow][i]);
      return INVALID_RESULT;
    }
  }

  return VALID_RESULT;
}

bool SnowCaliParamConfigSection::IsDuplicate(char *name, SNOWS snowVal) {
  std::map<std::string, SnowCaliParamConfigSection *>::iterator itr =
      g_snowCaliParamConfigs[snowVal].find(name);
  if (itr == g_snowCaliParamConfigs[snowVal].end()) {
    return false;
  } else {
    return true;
  }
}

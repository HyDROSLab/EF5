#include "SnowParamSetConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::map<std::string, SnowParamSetConfigSection *>
    g_snowParamSetConfigs[SNOW_QTY];

SnowParamSetConfigSection::SnowParamSetConfigSection(char *nameVal,
                                                     SNOWS snowVal) {
  strcpy(name, nameVal);
  currentGauge = NULL;
  currentParams = NULL;
  currentParamsSet = NULL;
  snow = snowVal;
  paramGrids.resize(numSnowParams[snow]);
}

SnowParamSetConfigSection::~SnowParamSetConfigSection() {}

char *SnowParamSetConfigSection::GetName() { return name; }

CONFIG_SEC_RET SnowParamSetConfigSection::ProcessKeyValue(char *name,
                                                          char *value) {
  int numParams = numSnowParams[snow];

  if (strcasecmp(name, "gauge") == 0) {

    TOLOWER(value);
    std::map<std::string, GaugeConfigSection *>::iterator itr =
        g_gaugeConfigs.find(value);
    if (itr == g_gaugeConfigs.end()) {
      ERROR_LOGF("Unknown gauge \"%s\" in parameter set!", value);
      return INVALID_RESULT;
    }

    if (currentGauge != NULL) {
      // Lets verify the settings for the old gauge and then replace it
      for (int i = 0; i < numParams; i++) {
        if (!currentParamsSet[i]) {
          ERROR_LOGF("Incomplete parameter set! Parameter \"%s\" was not given "
                     "a value.",
                     snowParamStrings[snow][i]);
          return INVALID_RESULT;
        }
      }
      delete[] currentParamsSet;
      paramSettings.insert(std::pair<GaugeConfigSection *, float *>(
          currentGauge, currentParams));
    }

    if (IsDuplicateGauge(itr->second)) {
      ERROR_LOGF("Duplicate gauge \"%s\" in parameter set!", value);
      return INVALID_RESULT;
    }

    currentGauge = itr->second;
    currentParams = new float[numParams];
    currentParamsSet = new bool[numParams];
    memset(currentParams, 0, sizeof(float) * numParams);
    memset(currentParamsSet, 0, sizeof(bool) * numParams);
  } else {
    // Lets see if this belongs to a parameter grid
    for (int i = 0; i < numParams; i++) {
      // printf("%i %i %s %s\n", model, i, modelParamStrings[model][i], name);
      if (strcasecmp(name, snowParamGridStrings[snow][i]) == 0) {
        paramGrids[i] = std::string(value);
        return VALID_RESULT;
      }
    }

    if (!currentGauge) {
      ERROR_LOGF("Got parameter %s without a gauge being set!", name);
      return INVALID_RESULT;
    }
    // Lets see if this belongs to a parameter scalar
    for (int i = 0; i < numParams; i++) {
      // printf("%i %i %s %s\n", model, i, modelParamStrings[model][i], name);
      if (strcasecmp(name, snowParamStrings[snow][i]) == 0) {

        if (currentParamsSet[i]) {
          ERROR_LOGF("Duplicate parameter \"%s\" in parameter set!", name);
          return INVALID_RESULT;
        }

        currentParams[i] = atof(value);
        currentParamsSet[i] = true;

        return VALID_RESULT;
      }
    }

    // We got here so we must not know what this parameter is!
    ERROR_LOGF("Unknown parameter name \"%s\".", name);
    return INVALID_RESULT;
  }
  return VALID_RESULT;
}

CONFIG_SEC_RET SnowParamSetConfigSection::ValidateSection() {
  int numParams = numSnowParams[snow];

  if (currentGauge != NULL) {
    // Lets verify the settings for the old gauge and then replace it
    for (int i = 0; i < numParams; i++) {
      if (!currentParamsSet[i]) {
        ERROR_LOGF(
            "Incomplete parameter set! Parameter \"%s\" was not given a value.",
            snowParamStrings[snow][i]);
        return INVALID_RESULT;
      }
    }
    delete[] currentParamsSet;
    paramSettings.insert(
        std::pair<GaugeConfigSection *, float *>(currentGauge, currentParams));
  }

  return VALID_RESULT;
}

bool SnowParamSetConfigSection::IsDuplicate(char *name, SNOWS snowVal) {
  std::map<std::string, SnowParamSetConfigSection *>::iterator itr =
      g_snowParamSetConfigs[snowVal].find(name);
  if (itr == g_snowParamSetConfigs[snowVal].end()) {
    return false;
  } else {
    return true;
  }
}

bool SnowParamSetConfigSection::IsDuplicateGauge(GaugeConfigSection *gaugeVal) {
  std::map<GaugeConfigSection *, float *>::iterator itr =
      paramSettings.find(gaugeVal);
  if (itr == paramSettings.end()) {
    return false;
  } else {
    return true;
  }
}

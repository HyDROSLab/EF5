#include "ParamSetConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::map<std::string, ParamSetConfigSection *> g_paramSetConfigs[MODEL_QTY];

ParamSetConfigSection::ParamSetConfigSection(char *nameVal, MODELS modelVal) {
  strcpy(name, nameVal);
  currentGauge = NULL;
  currentParams = NULL;
  currentParamsSet = NULL;
  model = modelVal;
  paramGrids.resize(numModelParams[model]);
}

ParamSetConfigSection::~ParamSetConfigSection() {}

char *ParamSetConfigSection::GetName() { return name; }

CONFIG_SEC_RET ParamSetConfigSection::ProcessKeyValue(char *name, char *value) {
  int numParams = numModelParams[model];

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
                     modelParamStrings[model][i]);
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
      if (strcasecmp(name, modelParamGridStrings[model][i]) == 0) {
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
      if (strcasecmp(name, modelParamStrings[model][i]) == 0) {

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

CONFIG_SEC_RET ParamSetConfigSection::ValidateSection() {
  int numParams = numModelParams[model];

  if (currentGauge != NULL) {
    // Lets verify the settings for the old gauge and then replace it
    for (int i = 0; i < numParams; i++) {
      if (!currentParamsSet[i]) {
        ERROR_LOGF(
            "Incomplete parameter set! Parameter \"%s\" was not given a value.",
            modelParamStrings[model][i]);
        return INVALID_RESULT;
      }
    }
    delete[] currentParamsSet;
    paramSettings.insert(
        std::pair<GaugeConfigSection *, float *>(currentGauge, currentParams));
  }

  return VALID_RESULT;
}

bool ParamSetConfigSection::IsDuplicate(char *name, MODELS modelVal) {
  std::map<std::string, ParamSetConfigSection *>::iterator itr =
      g_paramSetConfigs[modelVal].find(name);
  if (itr == g_paramSetConfigs[modelVal].end()) {
    return false;
  } else {
    return true;
  }
}

bool ParamSetConfigSection::IsDuplicateGauge(GaugeConfigSection *gaugeVal) {
  std::map<GaugeConfigSection *, float *>::iterator itr =
      paramSettings.find(gaugeVal);
  if (itr == paramSettings.end()) {
    return false;
  } else {
    return true;
  }
}

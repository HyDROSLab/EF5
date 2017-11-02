#include "CaliParamConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::map<std::string, CaliParamConfigSection *> g_caliParamConfigs[MODEL_QTY];

CaliParamConfigSection::CaliParamConfigSection(char *nameVal, MODELS modelVal) {
  strcpy(name, nameVal);
  gauge = NULL;
  objSet = false;
  model = modelVal;
  int numParams = numModelParams[model];
  modelParamMins = new float[numParams];
  modelParamMaxs = new float[numParams];
  modelParamInits = new float[numParams];
  paramsSet = new bool[numParams];
  memset(modelParamMins, 0, sizeof(float) * numParams);
  memset(modelParamMins, 0, sizeof(float) * numParams);
  memset(modelParamInits, 0, sizeof(float) * numParams);
  memset(paramsSet, 0, sizeof(bool) * numParams);

  // ARS Defaults
  ars_topNum = 10;
  ars_critObjScore = 0.0;
  ars_convergenceCriteria = 0.005;
  ars_burnInSets = 100;

  // DREAM defaults
  dream_ndraw = 10000;
}

CaliParamConfigSection::~CaliParamConfigSection() {
  delete[] modelParamMins;
  delete[] modelParamMaxs;
  delete[] modelParamInits;
}

char *CaliParamConfigSection::GetName() { return name; }

CONFIG_SEC_RET CaliParamConfigSection::ProcessKeyValue(char *name,
                                                       char *value) {
  int numParams = numModelParams[model];

  if (!strcasecmp(name, "gauge")) {

    TOLOWER(value);
    std::map<std::string, GaugeConfigSection *>::iterator itr =
        g_gaugeConfigs.find(value);
    if (itr == g_gaugeConfigs.end()) {
      ERROR_LOGF("Unknown gauge \"%s\" in parameter set!", value);
      return INVALID_RESULT;
    }

    gauge = itr->second;
  } else if (!strcasecmp(name, "objective")) {
    for (int i = 0; i < OBJECTIVE_QTY; i++) {
      if (!strcasecmp(value, objectiveStrings[i])) {
        objSet = true;
        objective = (OBJECTIVES)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown objective function option \"%s\"!", value);
    INFO_LOGF("Valid objective function options are \"%s\"", "NSCE, CC, SSE");
    return INVALID_RESULT;
  } else if (!strcasecmp(name, "ars_topnum")) {
    ars_topNum = atoi(value);
    return VALID_RESULT;
  } else if (!strcasecmp(name, "ars_critobjscore")) {
    ars_critObjScore = atof(value);
    return VALID_RESULT;
  } else if (!strcasecmp(name, "ars_convcriteria")) {
    ars_convergenceCriteria = atof(value);
    return VALID_RESULT;
  } else if (!strcasecmp(name, "ars_burninnum")) {
    ars_burnInSets = atoi(value);
    return VALID_RESULT;
  } else if (!strcasecmp(name, "dream_ndraw")) {
    dream_ndraw = atoi(value);
    return VALID_RESULT;
  } else {
    if (!gauge) {
      ERROR_LOGF("Got parameter %s without a gauge being set!", name);
      return INVALID_RESULT;
    }

    // Lets see if this belongs to a parameter
    for (int i = 0; i < numParams; i++) {
      if (strcasecmp(name, modelParamStrings[model][i]) == 0) {

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

CONFIG_SEC_RET CaliParamConfigSection::ValidateSection() {
  int numParams = numModelParams[model];

  if (!gauge) {
    ERROR_LOG("The gauge on which calibration is to be performed was not set!");
    return INVALID_RESULT;
  }

  if (!objSet) {
    ERROR_LOG("The objective function was not specified!");
    return INVALID_RESULT;
  }

  for (int i = 0; i < numParams; i++) {
    if (!paramsSet[i]) {
      ERROR_LOGF(
          "Incomplete parameter set! Parameter \"%s\" was not given a value.",
          modelParamStrings[model][i]);
      return INVALID_RESULT;
    }
  }

  return VALID_RESULT;
}

bool CaliParamConfigSection::IsDuplicate(char *name, MODELS modelVal) {
  std::map<std::string, CaliParamConfigSection *>::iterator itr =
      g_caliParamConfigs[modelVal].find(name);
  if (itr == g_caliParamConfigs[modelVal].end()) {
    return false;
  } else {
    return true;
  }
}

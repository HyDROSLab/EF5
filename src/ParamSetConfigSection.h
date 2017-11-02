#ifndef CONFIG_PARAMSET_SECTION_H
#define CONFIG_PARAMSET_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include <map>

class ParamSetConfigSection : public ConfigSection {

public:
  ParamSetConfigSection(char *nameVal, MODELS modelVal);
  ~ParamSetConfigSection();

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();
  std::map<GaugeConfigSection *, float *> *GetParamSettings() {
    return &paramSettings;
  }
  std::vector<std::string> *GetParamGrids() { return &paramGrids; }

  static bool IsDuplicate(char *name, MODELS modelVal);

private:
  bool IsDuplicateGauge(GaugeConfigSection *);

  char name[CONFIG_MAX_LEN];
  MODELS model;
  GaugeConfigSection *currentGauge;
  float *currentParams;
  bool *currentParamsSet;
  std::map<GaugeConfigSection *, float *> paramSettings;
  std::vector<std::string> paramGrids;
};

extern std::map<std::string, ParamSetConfigSection *> g_paramSetConfigs[];

#endif

#ifndef CONFIG_SNOWPARAMSET_SECTION_H
#define CONFIG_SNOWPARAMSET_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include <map>

class SnowParamSetConfigSection : public ConfigSection {

public:
  SnowParamSetConfigSection(char *nameVal, SNOWS snowVal);
  ~SnowParamSetConfigSection();

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();
  std::map<GaugeConfigSection *, float *> *GetParamSettings() {
    return &paramSettings;
  }
  std::vector<std::string> *GetParamGrids() { return &paramGrids; }

  static bool IsDuplicate(char *name, SNOWS snowVal);

private:
  bool IsDuplicateGauge(GaugeConfigSection *);

  char name[CONFIG_MAX_LEN];
  SNOWS snow;
  GaugeConfigSection *currentGauge;
  float *currentParams;
  bool *currentParamsSet;
  std::map<GaugeConfigSection *, float *> paramSettings;
  std::vector<std::string> paramGrids;
};

extern std::map<std::string, SnowParamSetConfigSection *>
    g_snowParamSetConfigs[];

#endif

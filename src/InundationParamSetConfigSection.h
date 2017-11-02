#ifndef CONFIG_INUNDATIONPARAMSET_SECTION_H
#define CONFIG_INUNDATIONPARAMSET_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include <map>

class InundationParamSetConfigSection : public ConfigSection {

public:
  InundationParamSetConfigSection(char *nameVal, INUNDATIONS inundationVal);
  ~InundationParamSetConfigSection();

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();
  std::map<GaugeConfigSection *, float *> *GetParamSettings() {
    return &paramSettings;
  }
  std::vector<std::string> *GetParamGrids() { return &paramGrids; }

  static bool IsDuplicate(char *name, INUNDATIONS inundationVal);

private:
  bool IsDuplicateGauge(GaugeConfigSection *);

  char name[CONFIG_MAX_LEN];
  INUNDATIONS inundation;
  GaugeConfigSection *currentGauge;
  float *currentParams;
  bool *currentParamsSet;
  std::map<GaugeConfigSection *, float *> paramSettings;
  std::vector<std::string> paramGrids;
};

extern std::map<std::string, InundationParamSetConfigSection *>
    g_inundationParamSetConfigs[];

#endif

#ifndef CONFIG_SNOWCALIPARAM_SECTION_H
#define CONFIG_SNOWCALIPARAM_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include "ObjectiveFunc.h"
#include <map>

class SnowCaliParamConfigSection : public ConfigSection {

public:
  SnowCaliParamConfigSection(char *nameVal, SNOWS routeVal);
  ~SnowCaliParamConfigSection();

  GaugeConfigSection *GetGauge() { return gauge; }
  float *GetParamMins() { return modelParamMins; }
  float *GetParamMaxs() { return modelParamMaxs; }
  float *GetParamInits() { return modelParamInits; }

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name, SNOWS snowVal);

private:
  char name[CONFIG_MAX_LEN];
  SNOWS snow;
  GaugeConfigSection *gauge;
  float *modelParamMins;
  float *modelParamMaxs;
  float *modelParamInits;
  bool *paramsSet;
};

extern std::map<std::string, SnowCaliParamConfigSection *>
    g_snowCaliParamConfigs[];

#endif

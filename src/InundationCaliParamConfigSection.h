#ifndef CONFIG_INUNDATIONCALIPARAM_SECTION_H
#define CONFIG_INUNDATIONCALIPARAM_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include "ObjectiveFunc.h"
#include <map>

class InundationCaliParamConfigSection : public ConfigSection {

public:
  InundationCaliParamConfigSection(char *nameVal, INUNDATIONS routeVal);
  ~InundationCaliParamConfigSection();

  GaugeConfigSection *GetGauge() { return gauge; }
  float *GetParamMins() { return modelParamMins; }
  float *GetParamMaxs() { return modelParamMaxs; }
  float *GetParamInits() { return modelParamInits; }

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name, INUNDATIONS inundationVal);

private:
  char name[CONFIG_MAX_LEN];
  INUNDATIONS inundation;
  GaugeConfigSection *gauge;
  float *modelParamMins;
  float *modelParamMaxs;
  float *modelParamInits;
  bool *paramsSet;
};

extern std::map<std::string, InundationCaliParamConfigSection *>
    g_inundationCaliParamConfigs[];

#endif

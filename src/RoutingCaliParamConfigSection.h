#ifndef CONFIG_ROUTINGCALIPARAM_SECTION_H
#define CONFIG_ROUTINGCALIPARAM_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include "ObjectiveFunc.h"
#include <map>

class RoutingCaliParamConfigSection : public ConfigSection {

public:
  RoutingCaliParamConfigSection(char *nameVal, ROUTES routeVal);
  ~RoutingCaliParamConfigSection();

  GaugeConfigSection *GetGauge() { return gauge; }
  float *GetParamMins() { return modelParamMins; }
  float *GetParamMaxs() { return modelParamMaxs; }
  float *GetParamInits() { return modelParamInits; }

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name, ROUTES routeVal);

private:
  char name[CONFIG_MAX_LEN];
  ROUTES route;
  GaugeConfigSection *gauge;
  float *modelParamMins;
  float *modelParamMaxs;
  float *modelParamInits;
  bool *paramsSet;
};

extern std::map<std::string, RoutingCaliParamConfigSection *>
    g_routingCaliParamConfigs[];

#endif

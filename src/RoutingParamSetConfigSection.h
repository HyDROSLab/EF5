#ifndef CONFIG_ROUTINGPARAMSET_SECTION_H
#define CONFIG_ROUTINGPARAMSET_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include <map>

class RoutingParamSetConfigSection : public ConfigSection {

public:
  RoutingParamSetConfigSection(char *nameVal, ROUTES routeVal);
  ~RoutingParamSetConfigSection();

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();
  std::map<GaugeConfigSection *, float *> *GetParamSettings() {
    return &paramSettings;
  }
  std::vector<std::string> *GetParamGrids() { return &paramGrids; }

  static bool IsDuplicate(char *name, ROUTES routeVal);

private:
  bool IsDuplicateGauge(GaugeConfigSection *);

  char name[CONFIG_MAX_LEN];
  ROUTES route;
  GaugeConfigSection *currentGauge;
  float *currentParams;
  bool *currentParamsSet;
  std::map<GaugeConfigSection *, float *> paramSettings;
  std::vector<std::string> paramGrids;
};

extern std::map<std::string, RoutingParamSetConfigSection *>
    g_routingParamSetConfigs[];

#endif

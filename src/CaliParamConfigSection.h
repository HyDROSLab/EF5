#ifndef CONFIG_CALIPARAM_SECTION_H
#define CONFIG_CALIPARAM_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "GaugeConfigSection.h"
#include "Model.h"
#include "ObjectiveFunc.h"
#include <map>

class CaliParamConfigSection : public ConfigSection {

public:
  CaliParamConfigSection(char *nameVal, MODELS modelVal);
  ~CaliParamConfigSection();

  OBJECTIVES GetObjFunc() { return objective; }
  GaugeConfigSection *GetGauge() { return gauge; }
  float *GetParamMins() { return modelParamMins; }
  float *GetParamMaxs() { return modelParamMaxs; }
  float *GetParamInits() { return modelParamInits; }

  // ARS
  int ARSGetTopNum() { return ars_topNum; }
  float ARSGetCritObjScore() { return ars_critObjScore; }
  float ARSGetConvCriteria() { return ars_convergenceCriteria; }
  int ARSGetBurnInSets() { return ars_burnInSets; }

  // DREAM
  int DREAMGetNDraw() { return dream_ndraw; }

  char *GetName();
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name, MODELS modelVal);

private:
  char name[CONFIG_MAX_LEN];
  bool objSet;
  MODELS model;
  OBJECTIVES objective;
  GaugeConfigSection *gauge;
  float *modelParamMins;
  float *modelParamMaxs;
  float *modelParamInits;
  bool *paramsSet;
  int ars_topNum;
  float ars_critObjScore;
  float ars_convergenceCriteria;
  int ars_burnInSets;
  int dream_ndraw;
};

extern std::map<std::string, CaliParamConfigSection *> g_caliParamConfigs[];

#endif

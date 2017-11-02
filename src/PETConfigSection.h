#ifndef CONFIG_PET_SECTION_H
#define CONFIG_PET_SECTION_H

#include "ConfigSection.h"
#include "DatedName.h"
#include "Defines.h"
#include "DistancePerTimeUnits.h"
#include "PETType.h"
#include "TimeUnit.h"
#include <map>
#include <string>

class PETConfigSection : public ConfigSection {

public:
  PETConfigSection();
  ~PETConfigSection();

  DatedName *GetFileName();
  TimeUnit *GetFreq() { return &freq; }
  char *GetLoc() { return loc; }
  TimeUnit *GetUnitTime() { return unit.GetTime(); }
  SUPPORTED_PET_TYPES GetType() { return type.GetType(); }
  bool IsTemperature() { return isTemp; }
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name);

private:
  bool locSet, freqSet, nameSet, unitSet, typeSet;
  bool isTemp;
  char loc[CONFIG_MAX_LEN];
  DatedName fileName;
  TimeUnit freq;
  DistancePerTimeUnits unit;
  PETType type;
};

extern std::map<std::string, PETConfigSection *> g_petConfigs;

#endif

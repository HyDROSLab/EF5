#ifndef CONFIG_TEMP_SECTION_H
#define CONFIG_TEMP_SECTION_H

#include "ConfigSection.h"
#include "DatedName.h"
#include "Defines.h"
#include "DistancePerTimeUnits.h"
#include "TempType.h"
#include "TimeUnit.h"
#include <map>
#include <string>

class TempConfigSection : public ConfigSection {

public:
  TempConfigSection();
  ~TempConfigSection();

  DatedName *GetFileName();
  TimeUnit *GetFreq() { return &freq; }
  char *GetLoc() { return loc; }
  char *GetDEM() { return dem; }
  TimeUnit *GetUnitTime() { return unit.GetTime(); }
  SUPPORTED_TEMP_TYPES GetType() { return type.GetType(); }
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name);

private:
  bool locSet, freqSet, nameSet, unitSet, typeSet, demSet;
  char loc[CONFIG_MAX_LEN];
  char dem[CONFIG_MAX_LEN];
  DatedName fileName;
  TimeUnit freq;
  DistancePerTimeUnits unit;
  TempType type;
};

extern std::map<std::string, TempConfigSection *> g_tempConfigs;

#endif

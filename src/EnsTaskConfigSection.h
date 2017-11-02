#ifndef CONFIG_ENSTASKCONFIG_SECTION_H
#define CONFIG_ENSTASKCONFIG_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "TaskConfigSection.h"
#include <string>
#include <vector>

class EnsTaskConfigSection : public ConfigSection {

public:
  EnsTaskConfigSection(const char *nameVal);
  ~EnsTaskConfigSection();

  char *GetName();
  RUNSTYLE GetRunStyle();

  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  std::vector<TaskConfigSection *> *GetTasks();

  static bool IsDuplicate(char *name);

private:
  bool styleSet;
  char name[CONFIG_MAX_LEN];
  RUNSTYLE style;
  std::vector<TaskConfigSection *> tasks;
};

extern std::map<std::string, EnsTaskConfigSection *> g_ensTaskConfigs;

#endif

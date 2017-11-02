#include "EnsTaskConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstring>

std::map<std::string, EnsTaskConfigSection *> g_ensTaskConfigs;

EnsTaskConfigSection::EnsTaskConfigSection(const char *nameVal) {
  strcpy(name, nameVal);
  styleSet = false;
}

EnsTaskConfigSection::~EnsTaskConfigSection() {}

char *EnsTaskConfigSection::GetName() { return name; }

RUNSTYLE EnsTaskConfigSection::GetRunStyle() { return style; }

CONFIG_SEC_RET EnsTaskConfigSection::ProcessKeyValue(char *name, char *value) {
  if (!strcasecmp(name, "style")) {
    for (int i = 0; i < STYLE_QTY; i++) {
      if (!strcasecmp(value, runStyleStrings[i])) {
        styleSet = true;
        style = (RUNSTYLE)i;
        return VALID_RESULT;
      }
    }
    ERROR_LOGF("Unknown run style option \"%s\"!", value);
    return INVALID_RESULT;
  } else if (strcasecmp(name, "task") == 0) {
    TOLOWER(value);
    std::map<std::string, TaskConfigSection *>::iterator itr =
        g_taskConfigs.find(value);
    if (itr == g_taskConfigs.end()) {
      ERROR_LOGF("Unknown task \"%s\" in basin!", value);
      return INVALID_RESULT;
    }
    tasks.push_back(itr->second);
  } else {
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

CONFIG_SEC_RET EnsTaskConfigSection::ValidateSection() {

  if (tasks.size() == 0) {
    ERROR_LOG("No tasks were defined!");
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

std::vector<TaskConfigSection *> *EnsTaskConfigSection::GetTasks() {
  return &tasks;
}

bool EnsTaskConfigSection::IsDuplicate(char *name) {
  std::map<std::string, EnsTaskConfigSection *>::iterator itr =
      g_ensTaskConfigs.find(name);
  if (itr == g_ensTaskConfigs.end()) {
    return false;
  } else {
    return true;
  }
}

#include "ExecuteConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstring>

ExecuteConfigSection *g_executeConfig = NULL;

ExecuteConfigSection::ExecuteConfigSection() {}

ExecuteConfigSection::~ExecuteConfigSection() {}

CONFIG_SEC_RET ExecuteConfigSection::ProcessKeyValue(char *name, char *value) {
  if (strcasecmp(name, "task") == 0) {
    TOLOWER(value);
    std::map<std::string, TaskConfigSection *>::iterator itr =
        g_taskConfigs.find(value);
    if (itr == g_taskConfigs.end()) {
      ERROR_LOGF("Unknown task \"%s\"!", value);
      return INVALID_RESULT;
    }
    tasks.push_back(itr->second);
  } else if (strcasecmp(name, "ensembletask") == 0) {
    TOLOWER(value);
    std::map<std::string, EnsTaskConfigSection *>::iterator itr =
        g_ensTaskConfigs.find(value);
    if (itr == g_ensTaskConfigs.end()) {
      ERROR_LOGF("Unknown ensemble task \"%s\"!", value);
      return INVALID_RESULT;
    }
    ensTasks.push_back(itr->second);
  } else {
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

CONFIG_SEC_RET ExecuteConfigSection::ValidateSection() {
  if (tasks.size() == 0) {
    ERROR_LOG("No tasks were defined!");
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

std::vector<TaskConfigSection *> *ExecuteConfigSection::GetTasks() {
  return &tasks;
}

std::vector<EnsTaskConfigSection *> *ExecuteConfigSection::GetEnsTasks() {
  return &ensTasks;
}

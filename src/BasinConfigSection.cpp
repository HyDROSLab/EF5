#include "BasinConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstring>

std::map<std::string, BasinConfigSection *> g_basinConfigs;

BasinConfigSection::BasinConfigSection(char *newName) { strcpy(name, newName); }

BasinConfigSection::~BasinConfigSection() {}

CONFIG_SEC_RET BasinConfigSection::ProcessKeyValue(char *name, char *value) {
  if (strcasecmp(name, "gauge") == 0) {
    TOLOWER(value);
    std::map<std::string, GaugeConfigSection *>::iterator itr =
        g_gaugeConfigs.find(value);
    if (itr == g_gaugeConfigs.end()) {
      ERROR_LOGF("Unknown gauge \"%s\" in basin!", value);
      return INVALID_RESULT;
    }
    if (IsDuplicateGauge(itr->second)) {
      ERROR_LOGF("Duplicate gauge \"%s\" in basin!", value);
      return INVALID_RESULT;
    }
    gauges.push_back(itr->second);
  } else {
    ERROR_LOGF("Unknown key value \"%s=%s\" in basin %s!", name, value,
               this->name);
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

CONFIG_SEC_RET BasinConfigSection::ValidateSection() {

  if (gauges.size() == 0) {
    ERROR_LOG("A basin was defined which contains no gauges!");
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

bool BasinConfigSection::IsDuplicateGauge(GaugeConfigSection *gauge) {

  // Scan the vector for duplicates
  for (std::vector<GaugeConfigSection *>::iterator itr = gauges.begin();
       itr != gauges.end(); itr++) {
    if (gauge == (*itr)) {
      return true;
    }
  }

  // No duplicates found!
  return false;
}

bool BasinConfigSection::IsDuplicate(char *name) {
  std::map<std::string, BasinConfigSection *>::iterator itr =
      g_basinConfigs.find(std::string(name));
  if (itr == g_basinConfigs.end()) {
    return false;
  } else {
    return true;
  }
}

#ifndef CONFIG_H
#define CONFIG_H

#include "ConfigSection.h"
#include "Defines.h"

enum CONFIG_PARSE_RESULTS {
  CONFIG_SUCCESS,
  CONFIG_OPENFAILED,
  CONFIG_READFAILED,
  CONFIG_INV_SECTION,
  CONFIG_INV_SECHEAD,
  CONFIG_INV_NAME,
  CONFIG_INV_NAMEKEY,
};

class Config {

public:
  Config(const char *configName);
  ~Config();

  CONFIG_PARSE_RESULTS ParseConfig();

private:
  ConfigSection *GetConfigSection(char *sectionName, char *sectionVal);

  char name[CONFIG_MAX_LEN];
};

#endif

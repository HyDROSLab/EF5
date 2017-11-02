#include "PETConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstring>
#include <string>

std::map<std::string, PETConfigSection *> g_petConfigs;

PETConfigSection::PETConfigSection() {
  locSet = false;
  freqSet = false;
  nameSet = false;
  typeSet = false;
  unitSet = false;
  isTemp = false;
}

PETConfigSection::~PETConfigSection() {}

DatedName *PETConfigSection::GetFileName() { return &fileName; }

CONFIG_SEC_RET PETConfigSection::ProcessKeyValue(char *name, char *value) {

  if (!strcasecmp(name, "type")) {
    SUPPORTED_PET_TYPES result = type.ParseType(value);
    if (result == PET_TYPE_QTY) {
      ERROR_LOGF("Unknown type option \"%s\"", value);
      INFO_LOGF("Valid PET type options are \"%s\"", type.GetTypes());
      return INVALID_RESULT;
    }
    typeSet = true;
  } else if (!strcasecmp(name, "unit")) {
    bool result = unit.ParseUnit(value);
    if (!result) {
      if (!strcasecmp(value, "c")) {
        isTemp = true;
        result = true;
      }
    }
    if (!result) {
      ERROR_LOGF("Unknown unit option \"%s\"", value);
      return INVALID_RESULT;
    }
    unitSet = true;
  } else if (!strcasecmp(name, "freq")) {
    SUPPORTED_TIME_UNITS result = freq.ParseUnit(value);
    if (result == TIME_UNIT_QTY) {
      ERROR_LOGF("Unknown frequency option \"%s\"", value);
      return INVALID_RESULT;
    }
    freqSet = true;
  } else if (!strcasecmp(name, "loc")) {
    strcpy(loc, value);
    locSet = true;
  } else if (!strcasecmp(name, "name")) {
    fileName.SetNameStr(value);
    nameSet = true;
  }
  return VALID_RESULT;
}

CONFIG_SEC_RET PETConfigSection::ValidateSection() {
  if (!typeSet) {
    ERROR_LOG("The type was not specified");
    return INVALID_RESULT;
  } else if (!unitSet) {
    ERROR_LOG("The unit was not specified");
    return INVALID_RESULT;
  } else if (!freqSet) {
    ERROR_LOG("The frequency was not specified");
    return INVALID_RESULT;
  } else if (!locSet) {
    ERROR_LOG("The location was not specified");
    return INVALID_RESULT;
  } else if (!nameSet) {
    ERROR_LOG("The file name was not specified");
    return INVALID_RESULT;
  }

  if (!fileName.ProcessNameLoose(&freq)) {
    ERROR_LOG("The file name was specified with improper resolution");
    return INVALID_RESULT;
  }

  return VALID_RESULT;
}

bool PETConfigSection::IsDuplicate(char *name) {
  std::map<std::string, PETConfigSection *>::iterator itr =
      g_petConfigs.find(std::string(name));
  if (itr == g_petConfigs.end()) {
    return false;
  } else {
    return true;
  }
}

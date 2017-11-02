#include "PETType.h"
#include <cstring>

const char *petTypeStrings[] = {
    "asc",
    "bif",
    "tif",
};

SUPPORTED_PET_TYPES PETType::GetType() { return type; }

SUPPORTED_PET_TYPES PETType::ParseType(char *typeStr) {
  SUPPORTED_PET_TYPES result = PET_TYPE_QTY;

  for (int i = 0; i < PET_TYPE_QTY; i++) {
    if (!strcasecmp(petTypeStrings[i], typeStr)) {
      result = (SUPPORTED_PET_TYPES)i;
      break;
    }
  }

  type = result;

  return result;
}

const char *PETType::GetTypes() { return "ASC, BIF, TIF"; }

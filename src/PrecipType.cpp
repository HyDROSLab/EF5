#include "PrecipType.h"
#include <cstring>

const char *precipTypeStrings[] = {
    "asc", "mrms", "trmmrt", "trmmv7", "bif", "tif",
};

SUPPORTED_PRECIP_TYPES PrecipType::GetType() { return type; }

SUPPORTED_PRECIP_TYPES PrecipType::ParseType(char *typeStr) {
  SUPPORTED_PRECIP_TYPES result = PRECIP_TYPE_QTY;

  for (int i = 0; i < PRECIP_TYPE_QTY; i++) {
    if (!strcasecmp(precipTypeStrings[i], typeStr)) {
      result = (SUPPORTED_PRECIP_TYPES)i;
      break;
    }
  }

  type = result;

  return result;
}

const char *PrecipType::GetTypes() {
  return "ASC, MRMS, TRMMRT, TRMMV7, BIF, TIF";
}

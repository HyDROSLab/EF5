#include "TempType.h"
#include <cstring>

const char *tempTypeStrings[] = {
    "asc",
    "tif",
};

SUPPORTED_TEMP_TYPES TempType::GetType() { return type; }

SUPPORTED_TEMP_TYPES TempType::ParseType(char *typeStr) {
  SUPPORTED_TEMP_TYPES result = TEMP_TYPE_QTY;

  for (int i = 0; i < TEMP_TYPE_QTY; i++) {
    if (!strcasecmp(tempTypeStrings[i], typeStr)) {
      result = (SUPPORTED_TEMP_TYPES)i;
      break;
    }
  }

  type = result;

  return result;
}

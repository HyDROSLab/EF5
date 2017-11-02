#include "DatedName.h"
#include "Messages.h"
#include <cstdio>
#include <cstring>

const char *datedNameStrings[] = {
    "YYYY", "MM", "DD", "HH", "UU", "SS",
};

const int datePartLen[] = {
    4, 2, 2, 2, 2, 2,
};

void DatedName::SetNameStr(const char *nameStr) {
  strcpy(inUseName, nameStr);
  memset(inUseTimeParts, 0, sizeof(bool) * TIME_UNIT_QTY);
}

bool DatedName::ProcessName(TimeUnit *freq) {
  resolution = freq->GetTimeUnit();

  // Verify that our name string has all of the necessary variables
  // for the resolution specified by frequency
  for (int i = 0; i <= resolution; i++) {
    timeParts[i] = strstr(inUseName, datedNameStrings[i]);
    if (timeParts[i] == NULL) {
      ERROR_LOGF("Expected to find \"%s\" in \"%s\" but it was not found!",
                 datedNameStrings[i], inUseName);
      return false;
    }
    inUseTimeParts[i] = true;
  }

  return true;
}

bool DatedName::ProcessNameLoose(TimeUnit *freq) {
  // resolution = freq->GetTimeUnit();
  resolution = SECONDS;

  // Verify that our name string has all of the necessary variables
  // for the resolution specified by frequency
  for (int i = 0; i <= resolution; i++) {
    timeParts[i] = strstr(inUseName, datedNameStrings[i]);
    if (timeParts[i] != NULL) {
      inUseTimeParts[i] = true;
    }
  }

  return true;
}

void DatedName::UpdateName(tm *ptm) {
  char dateParts[6][5];

  memset(dateParts, 0, 6 * 5); // Zero out the buffer memory
  sprintf(dateParts[0], "%04d", ptm->tm_year + 1900);
  sprintf(dateParts[1], "%02d", ptm->tm_mon + 1);
  sprintf(dateParts[2], "%02d", ptm->tm_mday);
  sprintf(dateParts[3], "%02d", ptm->tm_hour);
  sprintf(dateParts[4], "%02d", ptm->tm_min);
  sprintf(dateParts[5], "%02d", ptm->tm_sec);

  for (int i = 0; i <= resolution; i++) {
    if (inUseTimeParts[i]) {
      memcpy(timeParts[i], dateParts[i], datePartLen[i]);
    }
  }
}

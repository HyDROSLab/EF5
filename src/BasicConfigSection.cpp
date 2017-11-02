#include "BasicConfigSection.h"
#include "Messages.h"
#include <cstdio>
#include <cstring>

BasicConfigSection *g_basicConfig;

BasicConfigSection::BasicConfigSection() {
  DEMSet = false;
  DDMSet = false;
  FAMSet = false;
  projectionSet = false;
  esriDDMSet = false;
  selfFAMSet = false;
  artist[0] = 0;
  copyright[0] = 0;
}

BasicConfigSection::~BasicConfigSection() {}

char *BasicConfigSection::GetDEM() { return DEM; }

char *BasicConfigSection::GetDDM() { return DDM; }

char *BasicConfigSection::GetFAM() { return FAM; }

char *BasicConfigSection::GetArtist() { return artist; }

char *BasicConfigSection::GetCopyright() { return copyright; }

PROJECTIONS BasicConfigSection::GetProjection() { return projection; }

CONFIG_SEC_RET BasicConfigSection::ProcessKeyValue(char *name, char *value) {

  if (!strcasecmp(name, "dem")) {
    strcpy(DEM, value);
    DEMSet = true;
  } else if (!strcasecmp(name, "ddm")) {
    strcpy(DDM, value);
    DDMSet = true;
  } else if (!strcasecmp(name, "fam")) {
    strcpy(FAM, value);
    FAMSet = true;
  } else if (!strcasecmp(name, "author")) {
    strcpy(artist, value);
    for (unsigned int i = 0; i < strlen(artist); i++) {
      if (artist[i] == '_') {
        artist[i] = ' ';
      }
    }
  } else if (!strcasecmp(name, "copyright")) {
    strcpy(copyright, value);
    for (unsigned int i = 0; i < strlen(copyright); i++) {
      if (copyright[i] == '_') {
        copyright[i] = ' ';
      }
    }
  } else if (!strcasecmp(name, "proj")) {
    if (!strcasecmp(value, "geographic")) {
      projection = PROJECTION_GEOGRAPHIC;
      projectionSet = true;
    } else if (!strcasecmp(value, "laea")) {
      projection = PROJECTION_LAEA;
      projectionSet = true;
    } else {
      ERROR_LOGF("Unknown projection option \"%s\"", value);
      INFO_LOGF("Valid projection options are \"%s\"", "GEOGRAPHIC, LAEA");
      return INVALID_RESULT;
    }
  } else if (!strcasecmp(name, "esriddm")) {
    if (!strcasecmp(value, "true")) {
      esriDDM = true;
      esriDDMSet = true;
    } else if (!strcasecmp(value, "false")) {
      esriDDM = false;
      esriDDMSet = true;
    } else {
      ERROR_LOGF("Unknown ESRI DDM option \"%s\"", value);
      INFO_LOGF("Valid ESRI DDM options are \"%s\"", "TRUE, FALSE");
      return INVALID_RESULT;
    }
  } else if (!strcasecmp(name, "selffam")) {
    if (!strcasecmp(value, "true")) {
      selfFAM = true;
      selfFAMSet = true;
    } else if (!strcasecmp(value, "false")) {
      selfFAM = false;
      selfFAMSet = true;
    } else {
      ERROR_LOGF("Unknown Self FAM option \"%s\"", value);
      INFO_LOGF("Valid Self FAM options are \"%s\"", "TRUE, FALSE");
      return INVALID_RESULT;
    }
  } else {
    ERROR_LOGF("Unknown name-key pair \"%s\"", name);
    return INVALID_RESULT;
  }
  return VALID_RESULT;
}

CONFIG_SEC_RET BasicConfigSection::ValidateSection() {
  if (!DEMSet) {
    ERROR_LOG("The DEM was not specified");
    return INVALID_RESULT;
  } else if (!DDMSet) {
    ERROR_LOG("The DDM was not specified");
    return INVALID_RESULT;
  } else if (!FAMSet) {
    ERROR_LOG("The FAM was not specified");
    return INVALID_RESULT;
  } else if (!projectionSet) {
    ERROR_LOG("The projection was not specified");
    return INVALID_RESULT;
  } else if (!esriDDMSet) {
    ERROR_LOG("The type of DDM (ESRIDDM) was not specified");
    return INVALID_RESULT;
  } else if (!selfFAMSet) {
    ERROR_LOG("If the FAM includes the current grid cell in the accumulation "
              "(SELFFAM) was not specified");
    return INVALID_RESULT;
  }
  return VALID_RESULT;
}


#include "Config.h"
#include "BasicConfigSection.h"
#include "BasinConfigSection.h"
#include "CaliParamConfigSection.h"
#include "ConfigSection.h"
#include "EnsTaskConfigSection.h"
#include "ExecuteConfigSection.h"
#include "GaugeConfigSection.h"
#include "Messages.h"
#include "Model.h"
#include "PETConfigSection.h"
#include "ParamSetConfigSection.h"
#include "PrecipConfigSection.h"
#include "TaskConfigSection.h"
#include "TempConfigSection.h"
#include <cstdio>
#include <cstring>

Config *g_config = NULL;

enum CONFIG_PARSE_STATE {
  PARSE_NORMAL,
  PARSE_CPLUSPLUS_COMMENT,
  PARSE_C_COMMENT,
  PARSE_BASH_COMMENT,
  PARSE_NAME,
  PARSE_VALUE,
  PARSE_SECTION_NAME,
  PARSE_SECTION_VALUE,
};

Config::Config(const char *configName) {
  // Copy over the config name into the class variable and ensure it is null
  // terminated
  strncpy(name, configName, CONFIG_MAX_LEN);
  name[CONFIG_MAX_LEN - 1] = 0;
}

Config::~Config() {}

CONFIG_PARSE_RESULTS Config::ParseConfig() {

  FILE *configFile;
  size_t fileLen;
  char *buffer;

  configFile = fopen(name, "rb");
  if (configFile == NULL) {
    ERROR_LOGF("Failed to open configuration file %s\n", name);
    return CONFIG_OPENFAILED;
  }

  // Get file length
  fseek(configFile, 0, SEEK_END);
  fileLen = ftell(configFile);
  fseek(configFile, 0, SEEK_SET);

  // Read the entire file into buffer, +4 to give us extra room to play with!
  buffer = new char[fileLen + 4];
  size_t readLen = fread(buffer, 1, fileLen, configFile);
  if (readLen != fileLen) {
    ERROR_LOGF("Failed to read configuration file %s\n", name);
    delete[] buffer;
    return CONFIG_READFAILED;
  }

  // Fill the extra space with zeros
  buffer[fileLen] = 0;
  buffer[fileLen + 1] = 0;
  buffer[fileLen + 2] = 0;
  buffer[fileLen + 3] = 0;

  // Close the file, we're done with it
  fclose(configFile);

  // Start the process of parsing all the information
  int line = 1;
  char nameBuf[CONFIG_MAX_LEN] = "";
  char valBuf[CONFIG_MAX_LEN] = "";
  size_t i = 0, nameIdx = 0, valIdx = 0;
  CONFIG_PARSE_STATE state = PARSE_NORMAL;
  ConfigSection *currentSection = NULL;

  // This is the meat of the parsing here
  for (i = 0; i < fileLen; i++) {
    switch (state) {
    case PARSE_NORMAL: {
      if (buffer[i] == '/' && buffer[i + 1] == '/') {
        state = PARSE_CPLUSPLUS_COMMENT;
        i++;
        continue;
      } else if (buffer[i] == '/' && buffer[i + 1] == '*') {
        state = PARSE_C_COMMENT;
        i++;
        continue;
      } else if (buffer[i] == '#') {
        state = PARSE_BASH_COMMENT;
        continue;
      } else if (buffer[i] == '\r' || buffer[i] == '\t' || buffer[i] == ' ') {
        continue;
      } else if (buffer[i] == '\n') {
        line++;
        continue;
      } else if (buffer[i] == '[') {
        state = PARSE_SECTION_NAME;
        nameIdx = 0;
        valIdx = 0;
        continue;
      } else if (currentSection == NULL) {
        ERROR_LOGF("%s(%i): Invalid key-value outside of section", name, line);
        delete[] buffer;
        return CONFIG_INV_NAMEKEY;
      } else {
        state = PARSE_NAME;
        nameBuf[0] = buffer[i];
        nameIdx = 1;
        valIdx = 0;
        continue;
      }
      break;
    }
    case PARSE_CPLUSPLUS_COMMENT:
    case PARSE_BASH_COMMENT: {
      if (buffer[i] == '\n') {
        line++;
        state = PARSE_NORMAL;
        continue;
      } else {
        continue;
      }
      break;
    }
    case PARSE_C_COMMENT: {
      if (buffer[i] == '\n') {
        line++;
        continue;
      } else if (buffer[i] == '*' && buffer[i + 1] == '/') {
        i++;
        state = PARSE_NORMAL;
        continue;
      } else {
        continue;
      }
      break;
    }
    case PARSE_SECTION_NAME: {
      if (buffer[i] == ' ') {
        state = PARSE_SECTION_VALUE;
        nameBuf[nameIdx] = 0; // Add the null terminator to the name string
        continue;
      } else if (buffer[i] == ']') {
        state = PARSE_NORMAL;
        valBuf[0] = 0;
        nameBuf[nameIdx] = 0;
        // Allow old section to throw errors
        if (currentSection != NULL && currentSection->ValidateSection()) {
          ERROR_LOGF("%s(%i): Previous section not valid", name, line);
          delete[] buffer;
          return CONFIG_INV_SECTION;
        }
        currentSection = GetConfigSection(nameBuf, valBuf);
        if (currentSection == NULL) {
          ERROR_LOGF("%s(%i): Unknown section \"%s\"", name, line, nameBuf);
          delete[] buffer;
          return CONFIG_INV_SECHEAD;
        }
        continue;
      } else if (buffer[i] == '\n') {
        // Invalid case!
        ERROR_LOGF("%s(%i): Invalid section header contains newline", name,
                   line);
        delete[] buffer;
        return CONFIG_INV_SECHEAD;
      } else {
        nameBuf[nameIdx] = buffer[i];
        nameIdx++;
        if (nameIdx == CONFIG_MAX_LEN) {
          ERROR_LOGF("%s(%i): Section name exceeds max length of %i", name,
                     line, CONFIG_MAX_LEN);
          delete[] buffer;
          return CONFIG_INV_SECHEAD;
        }
        continue;
      }
      break;
    }
    case PARSE_SECTION_VALUE: {
      if (buffer[i] == ']') {
        state = PARSE_NORMAL;
        valBuf[valIdx] = 0; // Add the null terminator
        // Allow old section to throw errors
        if (currentSection != NULL && currentSection->ValidateSection()) {
          ERROR_LOGF("%s(%i): Previous section not valid", name, line);
          delete[] buffer;
          return CONFIG_INV_SECTION;
        }
        currentSection = GetConfigSection(nameBuf, valBuf);
        if (currentSection == NULL) {
          ERROR_LOGF("%s(%i): Unknown or invalid section \"%s\"", name, line,
                     nameBuf);
          delete[] buffer;
          return CONFIG_INV_SECHEAD;
        }
        continue;
      } else if (buffer[i] == '\n') {
        // Invalid case!
        ERROR_LOGF("%s(%i): Invalid section header contains newline", name,
                   line);
        delete[] buffer;
        return CONFIG_INV_SECHEAD;
      } else if (buffer[i] == '\r') {
        continue;
      } else {
        valBuf[valIdx] = buffer[i];
        valIdx++;
        if (valIdx == CONFIG_MAX_LEN) {
          ERROR_LOGF("%s(%i): Section name exceeds max length of %i", name,
                     line, CONFIG_MAX_LEN);
          delete[] buffer;
          return CONFIG_INV_SECHEAD;
        }
        continue;
      }
      break;
    }
    case PARSE_NAME: {
      if (buffer[i] == '=') {
        state = PARSE_VALUE;
        nameBuf[nameIdx] = 0; // Add null terminator
        continue;
      } else if (buffer[i] == '\n') {
        nameBuf[nameIdx] = 0; // Add null terminator
        ERROR_LOGF("%s(%i): Invalid key (%s) contains newline", name, line,
                   nameBuf);
        delete[] buffer;
        return CONFIG_INV_NAME;
      } else {
        nameBuf[nameIdx] = buffer[i];
        nameIdx++;
        if (nameIdx == CONFIG_MAX_LEN) {
          ERROR_LOGF("%s(%i): Key exceeds max length of %i", name, line,
                     CONFIG_MAX_LEN);
          delete[] buffer;
          return CONFIG_INV_NAME;
        }
        continue;
      }
      break;
    }
    case PARSE_VALUE: {
      if (buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\r') {
        state = PARSE_NORMAL;
        valBuf[valIdx] = 0; // Add null terminator
        if (buffer[i] == '\n') {
          line++;
        }
        // DEBUG_LOGF("Found %s with value %s", nameBuf, valBuf);
        if (currentSection->ProcessKeyValue(nameBuf, valBuf)) {
          // ProcessKeyValue is responsible for printing error messages
          ERROR_LOGF("%s(%i): Invalid key-value pair!", name, line);
          delete[] buffer;
          return CONFIG_INV_NAMEKEY;
        }
        continue;
      } else {
        valBuf[valIdx] = buffer[i];
        valIdx++;
        if (valIdx == CONFIG_MAX_LEN) {
          ERROR_LOGF("%s(%i): Value exceeds max length of %i", name, line,
                     CONFIG_MAX_LEN);
          delete[] buffer;
          return CONFIG_INV_NAMEKEY;
        }
        continue;
      }
      break;
    }
    default:
      INFO_LOGF("Line(%i): First unprocessed line!", line);
      return CONFIG_SUCCESS;
    }
  }

  if (state == PARSE_VALUE) {
    state = PARSE_NORMAL;
    valBuf[valIdx] = 0; // Add null terminator
    if (currentSection->ProcessKeyValue(nameBuf, valBuf)) {
      // ProcessKeyValue is responsible for printing error messages
      ERROR_LOGF("%s(%i): Invalid key-value pair!", name, line);
      delete[] buffer;
      return CONFIG_INV_NAMEKEY;
    }
  }

  delete[] buffer;

  if (currentSection != NULL && currentSection->ValidateSection()) {
    ERROR_LOGF("%s(%i): Previous section not valid", name, line);
    return CONFIG_INV_SECTION;
  }

  return CONFIG_SUCCESS;
}

ConfigSection *Config::GetConfigSection(char *sectionName, char *sectionVal) {
  ConfigSection *newSec = NULL;

  if (strcasecmp(sectionName, "basic") == 0) {
    g_basicConfig = new BasicConfigSection();
    newSec = g_basicConfig;
  } else if (strcasecmp(sectionName, "precipforcing") == 0) {
    TOLOWER(sectionVal);
    if (PrecipConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate Precip section \"%s\"!", sectionVal);
      return newSec;
    }
    PrecipConfigSection *preSec = new PrecipConfigSection();
    g_precipConfigs.insert(
        std::pair<std::string, PrecipConfigSection *>(sectionVal, preSec));
    newSec = preSec;
  } else if (strcasecmp(sectionName, "petforcing") == 0) {
    TOLOWER(sectionVal);
    if (PETConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate PET section \"%s\"!", sectionVal);
      return newSec;
    }
    PETConfigSection *petSec = new PETConfigSection();
    g_petConfigs.insert(
        std::pair<std::string, PETConfigSection *>(sectionVal, petSec));
    newSec = petSec;
  } else if (strcasecmp(sectionName, "tempforcing") == 0) {
    TOLOWER(sectionVal);
    if (TempConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate Temp section \"%s\"!", sectionVal);
      return newSec;
    }
    TempConfigSection *tempSec = new TempConfigSection();
    g_tempConfigs.insert(
        std::pair<std::string, TempConfigSection *>(sectionVal, tempSec));
    newSec = tempSec;
  } else if (strcasecmp(sectionName, "gauge") == 0) {
    TOLOWER(sectionVal);
    if (GaugeConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate Gauge section \"%s\"!", sectionVal);
      return newSec;
    }
    GaugeConfigSection *gaugeSec = new GaugeConfigSection(sectionVal);
    g_gaugeConfigs.insert(
        std::pair<std::string, GaugeConfigSection *>(sectionVal, gaugeSec));
    newSec = gaugeSec;
  } else if (strcasecmp(sectionName, "basin") == 0) {
    TOLOWER(sectionVal);
    if (BasinConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate Basin section \"%s\"!", sectionVal);
      return newSec;
    }
    BasinConfigSection *basinSec = new BasinConfigSection(sectionVal);
    g_basinConfigs.insert(
        std::pair<std::string, BasinConfigSection *>(sectionVal, basinSec));
    newSec = basinSec;
  } else if (strcasecmp(sectionName, "task") == 0) {
    TOLOWER(sectionVal);
    if (TaskConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate Task section \"%s\"!", sectionVal);
      return newSec;
    }
    TaskConfigSection *taskSec = new TaskConfigSection(sectionVal);
    g_taskConfigs.insert(
        std::pair<std::string, TaskConfigSection *>(sectionVal, taskSec));
    newSec = taskSec;
  } else if (strcasecmp(sectionName, "ensembletask") == 0) {
    TOLOWER(sectionVal);
    if (EnsTaskConfigSection::IsDuplicate(sectionVal)) {
      ERROR_LOGF("Duplicate Ensemble Task section \"%s\"!", sectionVal);
      return newSec;
    }
    EnsTaskConfigSection *taskSec = new EnsTaskConfigSection(sectionVal);
    g_ensTaskConfigs.insert(
        std::pair<std::string, EnsTaskConfigSection *>(sectionVal, taskSec));
    newSec = taskSec;
  } else if (strcasecmp(sectionName, "execute") == 0) {
    g_executeConfig = new ExecuteConfigSection();
    newSec = g_executeConfig;
  } else {
    TOLOWER(sectionVal);

    // Lets see if this belongs to a water balance model parameter set
    for (int i = 0; i < MODEL_QTY; i++) {
      if (strcasecmp(sectionName, modelParamSetStrings[i]) == 0) {
        if (ParamSetConfigSection::IsDuplicate(sectionVal, (MODELS)i)) {
          ERROR_LOGF("Duplicate ParamSet section \"%s\"!", sectionVal);
          return newSec;
        }
        ParamSetConfigSection *psSec =
            new ParamSetConfigSection(sectionVal, (MODELS)i);
        g_paramSetConfigs[i].insert(
            std::pair<std::string, ParamSetConfigSection *>(sectionVal, psSec));
        newSec = psSec;
        break;
      } else if (strcasecmp(sectionName, modelCaliParamStrings[i]) == 0) {
        if (CaliParamConfigSection::IsDuplicate(sectionVal, (MODELS)i)) {
          ERROR_LOGF("Duplicate CaliParam section \"%s\"!", sectionVal);
          return newSec;
        }
        CaliParamConfigSection *cpSec =
            new CaliParamConfigSection(sectionVal, (MODELS)i);
        g_caliParamConfigs[i].insert(
            std::pair<std::string, CaliParamConfigSection *>(sectionVal,
                                                             cpSec));
        newSec = cpSec;
        break;
      }
    }

    // Or maybe a routing parameter set
    for (int i = 0; i < ROUTE_QTY; i++) {
      if (strcasecmp(sectionName, routeParamSetStrings[i]) == 0) {
        if (RoutingParamSetConfigSection::IsDuplicate(sectionVal, (ROUTES)i)) {
          ERROR_LOGF("Duplicate Routing ParamSet section \"%s\"!", sectionVal);
          return newSec;
        }
        RoutingParamSetConfigSection *psSec =
            new RoutingParamSetConfigSection(sectionVal, (ROUTES)i);
        g_routingParamSetConfigs[i].insert(
            std::pair<std::string, RoutingParamSetConfigSection *>(sectionVal,
                                                                   psSec));
        newSec = psSec;
        break;
      } else if (strcasecmp(sectionName, routeCaliParamStrings[i]) == 0) {
        if (RoutingCaliParamConfigSection::IsDuplicate(sectionVal, (ROUTES)i)) {
          ERROR_LOGF("Duplicate Routing CaliParam section \"%s\"!", sectionVal);
          return newSec;
        }
        RoutingCaliParamConfigSection *cpSec =
            new RoutingCaliParamConfigSection(sectionVal, (ROUTES)i);
        g_routingCaliParamConfigs[i].insert(
            std::pair<std::string, RoutingCaliParamConfigSection *>(sectionVal,
                                                                    cpSec));
        newSec = cpSec;
        break;
      }
    }

    // Or maybe a snow parameter set
    for (int i = 0; i < SNOW_QTY; i++) {
      if (strcasecmp(sectionName, snowParamSetStrings[i]) == 0) {
        if (SnowParamSetConfigSection::IsDuplicate(sectionVal, (SNOWS)i)) {
          ERROR_LOGF("Duplicate Snow ParamSet section \"%s\"!", sectionVal);
          return newSec;
        }
        SnowParamSetConfigSection *psSec =
            new SnowParamSetConfigSection(sectionVal, (SNOWS)i);
        g_snowParamSetConfigs[i].insert(
            std::pair<std::string, SnowParamSetConfigSection *>(sectionVal,
                                                                psSec));
        newSec = psSec;
        break;
      } else if (strcasecmp(sectionName, snowCaliParamStrings[i]) == 0) {
        if (SnowCaliParamConfigSection::IsDuplicate(sectionVal, (SNOWS)i)) {
          ERROR_LOGF("Duplicate Snow CaliParam section \"%s\"!", sectionVal);
          return newSec;
        }
        SnowCaliParamConfigSection *cpSec =
            new SnowCaliParamConfigSection(sectionVal, (SNOWS)i);
        g_snowCaliParamConfigs[i].insert(
            std::pair<std::string, SnowCaliParamConfigSection *>(sectionVal,
                                                                 cpSec));
        newSec = cpSec;
        break;
      }
    }

    // Or maybe a inundation parameter set
    for (int i = 0; i < INUNDATION_QTY; i++) {
      if (strcasecmp(sectionName, inundationParamSetStrings[i]) == 0) {
        if (InundationParamSetConfigSection::IsDuplicate(sectionVal,
                                                         (INUNDATIONS)i)) {
          ERROR_LOGF("Duplicate Inundation ParamSet section \"%s\"!",
                     sectionVal);
          return newSec;
        }
        InundationParamSetConfigSection *psSec =
            new InundationParamSetConfigSection(sectionVal, (INUNDATIONS)i);
        g_inundationParamSetConfigs[i].insert(
            std::pair<std::string, InundationParamSetConfigSection *>(
                sectionVal, psSec));
        newSec = psSec;
        break;
      } else if (strcasecmp(sectionName, inundationCaliParamStrings[i]) == 0) {
        if (InundationCaliParamConfigSection::IsDuplicate(sectionVal,
                                                          (INUNDATIONS)i)) {
          ERROR_LOGF("Duplicate Inundation CaliParam section \"%s\"!",
                     sectionVal);
          return newSec;
        }
        InundationCaliParamConfigSection *cpSec =
            new InundationCaliParamConfigSection(sectionVal, (INUNDATIONS)i);
        g_inundationCaliParamConfigs[i].insert(
            std::pair<std::string, InundationCaliParamConfigSection *>(
                sectionVal, cpSec));
        newSec = cpSec;
        break;
      }
    }
  }

  return newSec;
}

#ifndef CONFIG_DAM_SECTION_H
#define CONFIG_DAM_SECTION_H

#include "ConfigSection.h"
#include "Defines.h"
#include "Grid.h"
#include "TimeSeries.h"
#include "TimeVar.h"
#include <map>
#include <string>

class DamConfigSection : public ConfigSection {

public:
  DamConfigSection(char *nameVal);
  ~DamConfigSection();

  char *GetName();
  float GetLat() { return lat; }
  float GetLon() { return lon; }
  long GetGridNodeIndex() { return gridNodeIndex; }
  bool GetUsed() { return used; }
  bool OutputTS() { return outputTS; }
  bool WantDA() { return wantDA; }
  bool WantCO() { return wantCO; }
  long GetFlowAccum() { return flowAccum; }
  bool HasObsFlowAccum() { return obsFlowAccumSet; }
  float GetObsFlowAccum() { return obsFlowAccum; }
  float GetVolume() { return volume; }
  GridLoc *GetGridLoc() { return &gridLoc; }
  float GetObserved(TimeVar *currentTime);
  float GetObserved(TimeVar *currentTime, float diff);
  void SetObservedValue(char *timeBuffer, float dataValue);
  void LoadTS();
  void SetGridNodeIndex(long newVal) { gridNodeIndex = newVal; }
  void SetLat(float newVal) { lat = newVal; }
  void SetLon(float newVal) { lon = newVal; }
  void SetCellX(long newX) { gridLoc.x = newX; }
  void SetCellY(long newY) { gridLoc.y = newY; }
  void SetUsed(bool newVal) { used = newVal; }
  void SetFlowAccum(long newVal) { flowAccum = newVal; }
  bool NeedsProjecting() { return latSet; }
  CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
  CONFIG_SEC_RET ValidateSection();

  static bool IsDuplicate(char *name);

private:
  bool obsSet, latSet, lonSet, xSet, ySet, obsFlowAccumSet, outputTSSet,
      volumeSet;
  bool outputTS, wantDA, wantCO;
  char observation[CONFIG_MAX_LEN];
  char name[CONFIG_MAX_LEN];
  float lat;
  float lon;
  float obsFlowAccum;
  float volume;
  TimeSeries obs;

  // These are for basin carving procedures!
  long flowAccum;
  bool used;
  GridLoc gridLoc;
  long gridNodeIndex;
};

extern std::map<std::string, DamConfigSection *> g_damConfigs;

#endif

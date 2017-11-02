#ifndef SNOW17_MODEL_H
#define SNOW17_MODEL_H

#include "ModelBase.h"

enum STATES_SNOW17 {
  STATE_SNOW17_ATI,
  STATE_SNOW17_WQ,
  STATE_SNOW17_WI,
  STATE_SNOW17_DEFICIT,
  STATE_SNOW17_QTY
};

struct Snow17GridNode : BasicGridNode {
  float params[PARAM_SNOW17_QTY];
  float states[STATE_SNOW17_QTY];

  float P_atm;
};

class Snow17Model : public SnowModel {

public:
  Snow17Model();
  ~Snow17Model();
  bool InitializeModel(std::vector<GridNode> *newNodes,
                       std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void InitializeStates(TimeVar *beginTime, char *statePath);
  void SaveStates(TimeVar *currentTime, char *statePath,
                  GridWriterFull *gridWriter);
  bool SnowBalance(float jday, float stepHours, std::vector<float> *precip,
                   std::vector<float> *temp, std::vector<float> *melt,
                   std::vector<float> *swe);
  const char *GetName() { return "snow17"; }

private:
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void SnowBalanceInt(GridNode *node, Snow17GridNode *cNode, float stepHours,
                      float jday, float precipIn, float tempIn, float *melt,
                      float *swe);

  std::vector<GridNode> *nodes;
  std::vector<Snow17GridNode> snowNodes;
};

#endif

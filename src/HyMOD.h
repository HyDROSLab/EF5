#ifndef HYMOD_H
#define HYMOD_H

#include "ModelBase.h"

struct HyMODGridNode : BasicGridNode {

  HyMODGridNode() { Xq = NULL; }

  float params[PARAM_HYMOD_QTY];

  // Params that aren't directly specified
  double CPar, b;
  int numQF;

  // States
  float XCuz, XHuz, Xs;
  float *Xq;

  // Local states
  float precipExcess;
  float dischargeQF, dischargeSF;
};

class HyMOD : public WaterBalanceModel {

public:
  HyMOD();
  ~HyMOD();
  bool InitializeModel(std::vector<GridNode> *newNodes,
                       std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void InitializeStates(TimeVar *beginTime, char *statePath);
  void SaveStates(TimeVar *currentTime, char *statePath,
                  GridWriterFull *gridWriter);
  bool WaterBalance(float stepHours, std::vector<float> *precip,
                    std::vector<float> *pet, std::vector<float> *fastFlow,
                    std::vector<float> *slowFlow,
                    std::vector<float> *soilMoisture);
  bool IsLumped() { return true; }
  const char *GetName() { return "hymod"; }

private:
  void WaterBalanceInt(GridNode *node, HyMODGridNode *cNode, float stepHours,
                       float precipIn, float petIn);
  void LocalRouteQF(GridNode *node, HyMODGridNode *cNode, float stepHours);
  void LocalRouteSF(GridNode *node, HyMODGridNode *cNode, float stepHours);
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings);

  std::vector<GridNode> *nodes;
  std::vector<HyMODGridNode> hymodNodes;
};

#endif

#ifndef SAC_H
#define SAC_H

#include "ModelBase.h"

struct SACGridNode : BasicGridNode {

  float params[PARAM_SAC_QTY];

  float UZTWC, UZFWC, LZTWC, LZFSC, LZFPC, ADIMC;
  float dischargeF, dischargeS;
};

class SAC : public WaterBalanceModel {

public:
  SAC();
  ~SAC();
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
  bool IsLumped() { return false; }
  const char *GetName() { return "sac"; }

private:
  void WaterBalanceInt(GridNode *node, SACGridNode *cNode, float stepHours,
                       float precipIn, float petIn);
  // void LocalRouteQF(GridNode *node, HyMODGridNode *cNode);
  // void LocalRouteSF(GridNode *node, HyMODGridNode *cNode);
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);

  std::vector<GridNode> *nodes;
  std::vector<SACGridNode> sacNodes;
};

#endif

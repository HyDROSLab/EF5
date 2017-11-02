#ifndef HP_MODEL_H
#define HP_MODEL_H

#include "ModelBase.h"

struct HPGridNode : BasicGridNode {
  float params[PARAM_HP_QTY];
};

class HPModel : public WaterBalanceModel {

public:
  HPModel();
  ~HPModel();
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
  const char *GetName() { return "hp"; }

private:
  void WaterBalanceInt(GridNode *node, HPGridNode *cNode, float stepHours,
                       float precipIn, float petIn, float *fastFlow,
                       float *slowFlow);
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);

  std::vector<GridNode> *nodes;
  std::vector<HPGridNode> hpNodes;
};

#endif

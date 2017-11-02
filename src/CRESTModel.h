#ifndef CREST_MODEL_H
#define CREST_MODEL_H

#include "ModelBase.h"

enum STATES_CREST { STATE_CREST_SM, STATE_CREST_QTY };

enum CREST_LAYER {
  CREST_LAYER_OVERLAND,
  CREST_LAYER_INTERFLOW,
  CREST_LAYER_QTY,
};

struct CRESTGridNode : BasicGridNode {
  float params[PARAM_CREST_QTY];
  float states[STATE_CREST_QTY];

  // These guys are the state variables
  // double soilMoisture; // Current depth of water filling available pore space

  // These are results of the water balance
  double excess[CREST_LAYER_QTY];
  double actET;
};

class CRESTModel : public WaterBalanceModel {

public:
  CRESTModel();
  ~CRESTModel();
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
  const char *GetName() { return "crest"; }

private:
  void WaterBalanceInt(GridNode *node, CRESTGridNode *cNode, float stepHours,
                       float precipIn, float petIn, float *fastFlow,
                       float *slowFlow);
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);

  std::vector<GridNode> *nodes;
  std::vector<CRESTGridNode> crestNodes;
};

#endif

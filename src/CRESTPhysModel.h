#ifndef CRESTPHYS_MODEL_H
#define CRESTPHYS_MODEL_H

#include "ModelBase.h"

enum STATES_CRESTPHYS { STATE_CRESTPHYS_SM,
                        STATE_CRESTPHYS_GW,
                        STATE_CRESTPHYS_QTY };

enum CRESTPHYS_LAYER {
  CRESTPHYS_LAYER_OVERLAND,
  CRESTPHYS_LAYER_INTERFLOW,
  CRESTPHYS_LAYER_BASEFLOW,
  CRESTPHYS_LAYER_QTY,
};

struct CRESTPHYSGridNode : BasicGridNode {
  float params[PARAM_CRESTPHYS_QTY];
  float states[STATE_CRESTPHYS_QTY];

  // These guys are the state variables
  // double soilMoisture; // Current depth of water filling available pore space

  // These are results of the water balance
  double excess[CRESTPHYS_LAYER_QTY];
  double actET;
};

class CRESTPHYSModel : public WaterBalanceModel {

public:
  CRESTPHYSModel();
  ~CRESTPHYSModel();
  bool InitializeModel(std::vector<GridNode> *newNodes,
                       std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void InitializeStates(TimeVar *beginTime, char *statePath);
  void SaveStates(TimeVar *currentTime, char *statePath,
                  GridWriterFull *gridWriter);
  bool WaterBalance(float stepHours,
                    std::vector<float> *precip,
                    std::vector<float> *pet,
                    std::vector<float> *fastFlow,
                    std::vector<float> *interFlow,
                    std::vector<float> *baseFlow,
                    std::vector<float> *soilMoisture,
                    std::vector<float> *groundwater);

  bool IsLumped() { return false; }
  const char *GetName() { return "crestphys"; }

private:
  void WaterBalanceInt(GridNode *node, CRESTPHYSGridNode *cNode, float stepHours,
                       float precipIn, float petIn, float *fastFlow,
                       float *interFlow, float *baseflow);
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);

  std::vector<GridNode> *nodes;
  std::vector<CRESTPHYSGridNode> crestphysNodes;
};

#endif

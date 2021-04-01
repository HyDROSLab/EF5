#ifndef LR_MODEL_H
#define LR_MODEL_H

#include "ModelBase.h"

enum LR_LAYER {
  LR_LAYER_OVERLAND,
  LR_LAYER_INTERFLOW,
  LR_LAYER_QTY,
};

struct LRGridNode : BasicGridNode {
  float params[PARAM_LINEAR_QTY];

  double slopeSqrt;

  double
      nexTime[LR_LAYER_QTY]; // This is a by product of computing cell routing
  LRGridNode *routeCNode[2][LR_LAYER_QTY]; // This is the node we route water to
  GridNode *routeNode[2][LR_LAYER_QTY];
  double routeAmount[2][LR_LAYER_QTY];
  double incomingWater[LR_LAYER_QTY];

  double reservoirs[LR_LAYER_QTY]; // CREST has two excess storage reservoirs
                                   // (overland & interflow)
};

class LRRoute : public RoutingModel {

public:
  LRRoute();
  ~LRRoute();
  bool InitializeModel(std::vector<GridNode> *newNodes,
                       std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void InitializeStates(TimeVar *beginTime, char *statePath,
                        std::vector<float> *fastFlow,
                        std::vector<float> *baseFlow,
                        std::vector<float> *interFlow);
  void SaveStates(TimeVar *currentTime, char *statePath,
                  GridWriterFull *gridWriter);
  bool Route(float stepHours, std::vector<float> *fastFlow,
             std::vector<float> *interFlow,
             std::vector<float> *baseFlow,
              std::vector<float> *discharge);
  float GetMaxSpeed() { return maxSpeed; }

private:
  void RouteInt(GridNode *node, LRGridNode *cNode, float fastFlow,
                float interFlow, float baseFlow);
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void InitializeRouting(float timeSeconds);
  float SetObsInflow(long index, float inflow);

  std::vector<GridNode> *nodes;
  std::vector<LRGridNode> lrNodes;
  float maxSpeed;
  bool initialized;
};

#endif

#ifndef SI_MODEL_H
#define SI_MODEL_H

#include "ModelBase.h"

struct InundationGridNode : BasicGridNode {
  float params[PARAM_SI_QTY];

  float elevation;
  float elevationChannel;
  float elevDiff;
  unsigned long channelIndex;
};

class SimpleInundation : public InundationModel {

public:
  SimpleInundation();
  ~SimpleInundation();
  bool InitializeModel(std::vector<GridNode> *newNodes,
                       std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  bool Inundation(std::vector<float> *discharge, std::vector<float> *depth);
  const char *GetName() { return "simpleinundation"; }

private:
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void InundationInt(GridNode *node, InundationGridNode *cNode,
                     float dischargeIn, float *depth);

  std::vector<GridNode> *nodes;
  std::vector<InundationGridNode> iNodes;
};

#endif

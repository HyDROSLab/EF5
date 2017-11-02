#ifndef VCI_MODEL_H
#define VCI_MODEL_H

#include "ModelBase.h"

struct VCILayer {
  float totalVolume;
  float totalArea;
  float height;
  size_t toIndex;
};

struct VCInundationGridNode : BasicGridNode {
  float params[PARAM_VCI_QTY];
  std::vector<long> gridIndicies;
  std::vector<VCILayer *> layers;
};

class VCInundation : public InundationModel {

public:
  VCInundation();
  ~VCInundation();
  bool InitializeModel(std::vector<GridNode> *newNodes,
                       std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  bool Inundation(std::vector<float> *discharge, std::vector<float> *depth);
  const char *GetName() { return "vcinundation"; }

private:
  void
  InitializeParameters(std::map<GaugeConfigSection *, float *> *paramSettings,
                       std::vector<FloatGrid *> *paramGrids);
  void ComputeLayers(size_t nodeIndex, GridNode *node,
                     VCInundationGridNode *cNode);

  std::vector<GridNode> *nodes;
  std::vector<VCInundationGridNode> iNodes;
};

#endif

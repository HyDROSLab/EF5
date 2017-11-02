#ifndef GRID_NODE_H
#define GRID_NODE_H

#include "GaugeConfigSection.h"
#include <vector>

#define INVALID_DOWNSTREAM_NODE -1ul

struct BasicGridNode {};

struct GridNode {
  long x;
  long y;
  RefLoc refLoc;
  float slope;
  long fac;
  float area;
  float contribArea;
  float horLen;
  bool channelGridCell;
  GaugeConfigSection *gauge;
  unsigned long downStreamNode;
  unsigned long index;
  unsigned long modelIndex;
  BasicGridNode *modelNode;
};

typedef std::vector<GridNode> GridNodeVec;

#endif

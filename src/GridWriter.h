#ifndef GRIDWRITER_H
#define GRIDWRITER_H

#include "AscGrid.h"
#include "Grid.h"
#include "GridNode.h"
#include "TifGrid.h"
#include <vector>

class GridWriter {
public:
  void Initialize(std::vector<GridNode> *nodes);
  void WriteGrid(std::vector<GridNode> *nodes, std::vector<float> *data,
                 const char *file, bool ascii = true);
  void WriteGrid(std::vector<GridNode> *nodes, std::vector<double> *data,
                 const char *file, bool ascii = true);

private:
  FloatGrid grid;
  long minX, maxX, minY, maxY;
};

#endif

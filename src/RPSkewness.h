#ifndef RPSKEWNESS_H
#define RPSKEWNESS_H

#include "GridNode.h"
#include <vector>

struct RPData {
  float q1;
  float q2;
  float q5;
  float q10;
  float q25;
  float q50;
  float q100;
  float q200;
};

bool ReadLP3File(char *file, std::vector<GridNode> *nodes,
                 std::vector<float> *lp3Vals);
void CalcLP3Vals(std::vector<float> *stdGrid, std::vector<float> *avgGrid,
                 std::vector<float> *scGrid, std::vector<RPData> *rpData,
                 std::vector<GridNode> *nodes);
float GetReturnPeriod(float q, RPData *rpData);

#endif

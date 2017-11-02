#ifndef OBJECTIVE_FUNC_H
#define OBJECTIVE_FUNC_H

#include <vector>

enum OBJECTIVES {
  OBJECTIVE_NSCE,
  OBJECTIVE_CC,
  OBJECTIVE_SSE,
  OBJECTIVE_QTY,
};

enum OBJECTIVE_GOAL {
  OBJECTIVE_GOAL_MAXIMIZE,
  OBJECTIVE_GOAL_MINIMIZE,
};

extern const char *objectiveStrings[];
extern const OBJECTIVE_GOAL objectiveGoals[];

float CalcObjFunc(std::vector<float> *obs, std::vector<float> *sim,
                  OBJECTIVES obj);

#endif

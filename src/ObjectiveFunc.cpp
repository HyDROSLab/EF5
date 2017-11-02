#include "ObjectiveFunc.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

const char *objectiveStrings[] = {"nsce", "cc", "sse"};

const OBJECTIVE_GOAL objectiveGoals[] = {
    OBJECTIVE_GOAL_MAXIMIZE,
    OBJECTIVE_GOAL_MAXIMIZE,
    OBJECTIVE_GOAL_MINIMIZE,
};

// Internal functions for calculating actual objective function scores
static float CalcNSCE(std::vector<float> *obs, std::vector<float> *sim);
static float CalcCC(std::vector<float> *obs, std::vector<float> *sim);
static float CalcSSE(std::vector<float> *obs, std::vector<float> *sim);

// This is the main function for calculating objective functions, everything
// passes through here first.
float CalcObjFunc(std::vector<float> *obs, std::vector<float> *sim,
                  OBJECTIVES obj) {

  switch (obj) {
  case OBJECTIVE_NSCE:
    return CalcNSCE(obs, sim);
  case OBJECTIVE_CC:
    return CalcCC(obs, sim);
  case OBJECTIVE_SSE:
    return CalcSSE(obs, sim);
  default:
    return 0;
  }
}

float CalcNSCE(std::vector<float> *obs, std::vector<float> *sim) {

  float obsMean = 0, obsAcc = 0, simAcc = 0, validQs = 0;
  size_t totalTimeSteps = obs->size();
  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    if ((*obs)[tsIndex] == (*obs)[tsIndex] &&
        (*sim)[tsIndex] == (*sim)[tsIndex]) {
      obsMean += (*obs)[tsIndex];
      validQs++;
    }
  }

  obsMean /= validQs;

  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    if ((*obs)[tsIndex] == (*obs)[tsIndex] &&
        (*sim)[tsIndex] == (*sim)[tsIndex]) {
      // printf("%f %f\n", (*obs)[tsIndex], (*sim)[tsIndex]);
      obsAcc += powf((*obs)[tsIndex] - obsMean, 2.0);
      simAcc += powf((*obs)[tsIndex] - (*sim)[tsIndex], 2.0);
    }
  }

  float result = 1.0 - (simAcc / obsAcc);
  if (result == result) {
    return result;
  } else {
    return -10000000000.0;
  }
}

float CalcCC(std::vector<float> *obs, std::vector<float> *sim) {

  float obsMean = 0, simMean = 0, obsAcc2 = 0, obsAcc = 0, simAcc = 0;
  size_t validQs = 0, totalTimeSteps = obs->size();
  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    if ((*obs)[tsIndex] == (*obs)[tsIndex] &&
        (*sim)[tsIndex] == (*sim)[tsIndex]) {
      obsMean += (*obs)[tsIndex];
      simMean += (*sim)[tsIndex];
      validQs++;
    }
  }

  obsMean /= validQs;
  simMean /= validQs;

  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    if ((*obs)[tsIndex] == (*obs)[tsIndex] &&
        (*sim)[tsIndex] == (*sim)[tsIndex]) {
      obsAcc += pow((*obs)[tsIndex] - obsMean, 2);
      simAcc += pow((*sim)[tsIndex] - simMean, 2);
      obsAcc2 += (((*obs)[tsIndex] - obsMean) * ((*sim)[tsIndex] - simMean));
    }
  }

  obsAcc = sqrt(obsAcc);
  simAcc = sqrt(simAcc);

  return obsAcc2 / (obsAcc * simAcc);
}

float CalcSSE(std::vector<float> *obs, std::vector<float> *sim) {

  float sse = 0;
  size_t totalTimeSteps = obs->size();

  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    if ((*obs)[tsIndex] == (*obs)[tsIndex] &&
        (*sim)[tsIndex] == (*sim)[tsIndex]) {
      sse += pow((*obs)[tsIndex] - (*sim)[tsIndex], 2);
    }
  }

  return sse;
}

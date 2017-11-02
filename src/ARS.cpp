#include "ARS.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>

void ARS::Initialize(CaliParamConfigSection *caliParamConfigNew,
                     RoutingCaliParamConfigSection *routingCaliParamConfigNew,
                     SnowCaliParamConfigSection *snowCaliParamConfigNew,
                     int numParamsWBNew, int numParamsRNew, int numParamsSNew,
                     Simulator *simNew) {
  caliParamConfig = caliParamConfigNew;
  routingCaliParamConfig = routingCaliParamConfigNew;
  numParamsWB = numParamsWBNew;
  numParamsR = numParamsR;
  numParams = numParamsWBNew + numParamsRNew;
  sim = simNew;

  // Create storage arrays
  minParams = new float[numParams];
  maxParams = new float[numParams];
  currentParams = new float[numParams];

  // Stuff from CaliParamConfigSection
  memcpy(minParams, caliParamConfig->GetParamMins(), sizeof(float) * numParams);
  memcpy(maxParams, caliParamConfig->GetParamMaxs(), sizeof(float) * numParams);
  goal = objectiveGoals[caliParamConfig->GetObjFunc()];

  // Configurable Parameters
  topNum = caliParamConfig->ARSGetTopNum();
  minObjScore = caliParamConfig->ARSGetCritObjScore();
  convergenceCriteria = caliParamConfig->ARSGetConvCriteria();
  burnInSets = caliParamConfig->ARSGetBurnInSets();

  // Initialize vars & RNG
  totalSets = 0;
  goodSets = 0;
#ifdef WIN32
  srand(time(NULL));
#else
  srand48(time(NULL));
#endif
}

void ARS::CalibrateParams() {

  float objScore;
  float scoreDiff = 0;

  while (goodSets < burnInSets || scoreDiff > convergenceCriteria) {

    // Generate new parameters
    for (int i = 0; i < numParams; i++) {
#ifdef WIN32
      float randVal = ((float)rand()) / RAND_MAX;
#else
      float randVal = drand48(); //((float)rand()) / RAND_MAX;
#endif
      currentParams[i] = minParams[i] + (maxParams[i] - minParams[i]) * randVal;
    }

    objScore = sim->SimulateForCali(currentParams);
    // printf("%f\n", objScore);

    totalSets++;

    if (!(totalSets % 500)) {
      printf("Total sets %i, good sets %i!\n", totalSets, goodSets);
    }

    if (((goal == OBJECTIVE_GOAL_MAXIMIZE) && objScore < minObjScore) ||
        ((goal == OBJECTIVE_GOAL_MINIMIZE) && objScore > minObjScore)) {
      continue;
    } else {
      // This is a good parameter set, count it towards the burn in total!
      goodSets++;
    }

    bool insertedParams = false;
    for (std::list<ARS_INFO *>::iterator itr = topSets.begin();
         itr != topSets.end(); itr++) {
      ARS_INFO *current = *itr;
      // Add this sucker here, it is a winner!
      if (((goal == OBJECTIVE_GOAL_MAXIMIZE) && objScore > current->objScore) ||
          ((goal == OBJECTIVE_GOAL_MINIMIZE) && objScore < current->objScore)) {
        ARS_INFO *newInfo = new ARS_INFO;
        newInfo->params = new float[numParams];
        memcpy(newInfo->params, currentParams, sizeof(float) * numParams);
        newInfo->objScore = objScore;
        topSets.insert(itr, newInfo);
        insertedParams = true;
        break;
      }
    }

    // Lets see if the top number of sets hasn't filled yet, if so add it to the
    // back
    if (!insertedParams && topSets.size() < topNum) {
      ARS_INFO *newInfo = new ARS_INFO;
      newInfo->params = new float[numParams];
      memcpy(newInfo->params, currentParams, sizeof(float) * numParams);
      newInfo->objScore = objScore;
      topSets.push_back(newInfo);
      insertedParams = true;
    }

    // Ensure that our list of good parameter sets only contains topNum
    if (topSets.size() > topNum) {
      ARS_INFO *current = topSets.back();
      delete[] current->params;
      delete current;
      topSets.pop_back();
    }

    // Update the min and max values!
    if (goodSets > burnInSets && insertedParams) {

      for (int i = 0; i < numParams; i++) {
        minParams[i] = 9999;
        maxParams[i] = 0;
      }

      for (std::list<ARS_INFO *>::iterator itr = topSets.begin();
           itr != topSets.end(); itr++) {
        ARS_INFO *current = *itr;
        for (int i = 0; i < numParams; i++) {
          if (current->params[i] < minParams[i]) {
            minParams[i] = current->params[i];
          }
          if (current->params[i] > maxParams[i]) {
            maxParams[i] = current->params[i];
          }
        }
      }
    }

    ARS_INFO *top = topSets.front();
    ARS_INFO *bottom = topSets.back();
    if (insertedParams) {
      scoreDiff = top->objScore - bottom->objScore;
      printf("ConvC %f (%f, %f), total runs %i, good runs %i\n",
             (top->objScore - bottom->objScore), top->objScore,
             bottom->objScore, totalSets, goodSets);
    }
  }
}

void ARS::WriteOutput(char *outputFile, MODELS model, ROUTES route) {
  FILE *file = fopen(outputFile, "w");

  fprintf(file, "%s", "Rank,ObjFunc,");
  for (int i = 0; i < numParams; i++) {
    fprintf(file, "%s%s", modelParamStrings[model][i],
            (i != (numParams - 1)) ? "," : "\n");
  }

  int index = 0;
  for (std::list<ARS_INFO *>::iterator itr = topSets.begin();
       itr != topSets.end(); itr++) {
    ARS_INFO *current = *itr;

    fprintf(file, "%i,%f,", index, current->objScore);

    for (int i = 0; i < numParams; i++) {
      fprintf(file, "%f%s", current->params[i],
              (i != (numParams - 1)) ? "," : "\n");
    }

    index++;
  }

  fprintf(file, "%s", "\n\n\n");

  ARS_INFO *current = *(topSets.begin());
  for (int i = 0; i < numParams; i++) {
    fprintf(file, "%s=%f\n", modelParamStrings[model][i], current->params[i]);
  }

  fclose(file);
}

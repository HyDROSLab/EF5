#ifndef ARS_H
#define ARS_H

#include "Calibrate.h"
#include "Model.h"
#include <list>

struct ARS_INFO {
  float objScore;
  float *params;
};

class ARS : public Calibrate {
public:
  // ARS(int numParamsNew, int topNumNew, float *currentParamsNew, float
  // *minParamsNew, float *maxParamsNew, OBJECTIVE_GOAL goalNew);
  void Initialize(CaliParamConfigSection *caliParamConfigNew,
                  RoutingCaliParamConfigSection *routingCaliParamConfigNew,
                  SnowCaliParamConfigSection *snowCaliParamConfigNew,
                  int numParamsWBNew, int numParamsRNew, int numParamsSNew,
                  Simulator *simNew);
  void CalibrateParams();
  void WriteOutput(char *outputFile, MODELS model, ROUTES route);

private:
  float *minParams;
  float *maxParams;
  float *currentParams;
  OBJECTIVE_GOAL goal;
  std::list<ARS_INFO *> topSets;
  int numParams;
  unsigned int topNum;
  float minObjScore;
  float convergenceCriteria;
  int goodSets;
  int burnInSets;
  int totalSets;
};

#endif

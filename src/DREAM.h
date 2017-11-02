#ifndef DREAM_H
#define DREAM_H

#include "Calibrate.h"
#include "Model.h"
#include "dream_functions.h"
#include "dream_variables.h"
#include "misc_functions.h"

class DREAM : public Calibrate {
public:
  void Initialize(CaliParamConfigSection *caliParamConfigNew,
                  RoutingCaliParamConfigSection *routingCaliParamConfigNew,
                  SnowCaliParamConfigSection *snowCaliParamConfigNew,
                  int numParamsWBNew, int numParamsRNew, int numParamsSNew,
                  Simulator *simNew);
  void Initialize(CaliParamConfigSection *caliParamConfigNew, int numParamsNew,
                  float *paramMins, float *paramMaxs,
                  std::vector<Simulator> *ensSimsNew,
                  std::vector<int> *paramsPerSimNew);
  void CalibrateParams();
  void WriteOutput(char *outputFile, MODELS model, ROUTES route, SNOWS snow);

private:
  void CompDensity(float **p, float *log_p, float **x,
                   struct DREAM_Parameters *MCMC, struct Model_Input *Input,
                   int option);

  float *minParams;
  float *maxParams;
  struct DREAM_Parameters *pointerMCMC;
  struct Model_Input *pointerInput;
  OBJECTIVE_GOAL goal;
  const char *objectiveString;
  bool isEnsemble;
  std::vector<Simulator> *ensSims;
  std::vector<int> *paramsPerSim;

  int post_Sequences;
  struct DREAM_Variables *pointerRUNvar;
};

#endif

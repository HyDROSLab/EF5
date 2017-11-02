#ifndef CALIBRATE_H
#define CALIBRATE_H

#include "CaliParamConfigSection.h"
#include "ObjectiveFunc.h"
#include "Simulator.h"

class Calibrate {
public:
  virtual void
  Initialize(CaliParamConfigSection *caliParamConfigNew,
             RoutingCaliParamConfigSection *routingCaliParamConfigNew,
             SnowCaliParamConfigSection *snowCaliParamConfigNew,
             int numParamsWBNew, int numParamsRNew, int numParamsSNew,
             Simulator *simNew) = 0;
  virtual void CalibrateParams() = 0;

protected:
  Simulator *sim;
  int numParams, numParamsWB, numParamsR, numParamsS;
  CaliParamConfigSection *caliParamConfig;
  RoutingCaliParamConfigSection *routingCaliParamConfig;
  SnowCaliParamConfigSection *snowCaliParamConfig;
};

#endif

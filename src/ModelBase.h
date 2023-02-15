#ifndef MODELBASE_H
#define MODELBASE_H

#include "BasicGrids.h"
#include "GridWriterFull.h"
#include "ParamSetConfigSection.h"
#include "TimeUnit.h"
#include <vector>

class Model {

public:
  virtual bool
  InitializeModel(TimeUnit *timeStep, std::vector<GridNode> *nodes,
                  std::map<GaugeConfigSection *, float *> *paramSettings,
                  std::vector<FloatGrid *> *paramGrids) = 0;
  virtual void InitializeStates(TimeVar *beginTime, char *statePath) = 0;
  virtual void SaveStates(TimeVar *currentTime, char *statePath,
                          GridWriterFull *gridWriter) = 0;
  virtual bool RunTimeStep(long index, float stepHours,
                           std::vector<float> *precip, std::vector<float> *pet,
                           std::vector<float> *discharge,
                           std::vector<float> *soilMoisture,
                           std::vector<float> *groundwater) = 0;
  virtual bool IsLumped() = 0;
  virtual const char *GetName() = 0;
};

class WaterBalanceModel {

public:
  virtual bool
  InitializeModel(std::vector<GridNode> *nodes,
                  std::map<GaugeConfigSection *, float *> *paramSettings,
                  std::vector<FloatGrid *> *paramGrids) = 0;
  virtual void InitializeStates(TimeVar *beginTime, char *statePath) = 0;
  virtual void SaveStates(TimeVar *currentTime, char *statePath,
                          GridWriterFull *gridWriter) = 0;
  virtual bool WaterBalance(float stepHours,
                            std::vector<float> *precip,
                            std::vector<float> *pet,
                            std::vector<float> *fastFlow,
                            std::vector<float> *interFlow,
                            std::vector<float> *baseFlow,
                            std::vector<float> *soilMoisture,
                            std::vector<float> *groundwater) = 0;
  virtual bool IsLumped() = 0;
  virtual const char *GetName() = 0;
};

class RoutingModel {

public:
  virtual bool
  InitializeModel(std::vector<GridNode> *nodes,
                  std::map<GaugeConfigSection *, float *> *paramSettings,
                  std::vector<FloatGrid *> *paramGrids) = 0;
  virtual void InitializeStates(TimeVar *beginTime, char *statePath,
                                std::vector<float> *fastFlow,
                                std::vector<float> *interFlow,
                                std::vector<float> *baseFlow) = 0;
  virtual void SaveStates(TimeVar *currentTime, char *statePath,
                          GridWriterFull *gridWriter) = 0;
  virtual bool Route(float stepHours, std::vector<float> *fastFlow,
                     std::vector<float> *interFlow,
                     std::vector<float> *baseFlow,
                     std::vector<float> *discharge) = 0;
  virtual float GetMaxSpeed() = 0;
  virtual float SetObsInflow(long index, float inflow) = 0;
};

class SnowModel {

public:
  virtual bool
  InitializeModel(std::vector<GridNode> *nodes,
                  std::map<GaugeConfigSection *, float *> *paramSettings,
                  std::vector<FloatGrid *> *paramGrids) = 0;
  virtual void InitializeStates(TimeVar *beginTime, char *statePath) = 0;
  virtual void SaveStates(TimeVar *currentTime, char *statePath,
                          GridWriterFull *gridWriter) = 0;
  virtual bool SnowBalance(float jday, float stepHours,
                           std::vector<float> *precip, std::vector<float> *temp,
                           std::vector<float> *melt,
                           std::vector<float> *swe) = 0;
  virtual const char *GetName() = 0;
};

class InundationModel {

public:
  virtual bool
  InitializeModel(std::vector<GridNode> *nodes,
                  std::map<GaugeConfigSection *, float *> *paramSettings,
                  std::vector<FloatGrid *> *paramGrids) = 0;
  virtual bool Inundation(std::vector<float> *discharge,
                          std::vector<float> *depth) = 0;
  virtual const char *GetName() = 0;
};

#endif

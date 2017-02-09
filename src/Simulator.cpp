#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#if _OPENMP
#include <omp.h>
#endif
#include <cmath>
#include <string.h>
#include <zlib.h>
#include "Messages.h"
#include "BasicGrids.h"
#include "CRESTModel.h"
#include "HyMOD.h"
#include "SAC.h"
#include "HPModel.h"
#include "LinearRoute.h"
#include "KinematicRoute.h"
#include "Snow17Model.h"
#include "SimpleInundation.h"
#include "VCInundation.h"
#include "PETConfigSection.h"
#include "PrecipConfigSection.h"
#include "TifGrid.h"
#include "GridWriterFull.h"
#include "GriddedOutput.h"
#include "Simulator.h"

bool Simulator::Initialize(TaskConfigSection *taskN) {
  
  task = taskN;
  
  if (!InitializeBasic(task)) {
    return false;
  }
  
  if (task->GetRunStyle() == STYLE_SIMU || task->GetRunStyle() == STYLE_SIMU_RP || task->GetRunStyle() == STYLE_BASIN_AVG) {
    // We are a simulation run
    if (!InitializeSimu(task)) {
      return false;
    }
  } else {
    // Must be a calibration run
    if (!InitializeCali(task)) {
      return false;
    }
  }
  
  LoadDAFile(task);
  
  // Everything succeeded!
  return true;
}

bool Simulator::InitializeBasic(TaskConfigSection *task) {
  // Initialize time step information
  inLR = false;
  timeStep = task->GetTimeStep();
  timeStepSR = task->GetTimeStep();
  timeStepLR = task->GetTimeStepLR();
  timeStepPrecip = task->GetPrecipSec()->GetFreq();
  if (task->GetQPFSec()) {
    hasQPF = true;
    timeStepQPF = task->GetQPFSec()->GetFreq();
  } else {
    hasQPF = false;
  }
  timeStepPET = task->GetPETSec()->GetFreq();
  
  if (task->GetSnow() != SNOW_QTY) {
    timeStepTemp = task->GetTempSec()->GetFreq();
    if (task->GetTempFSec()) {
      hasTempF = true;
      timeStepTempF = task->GetTempFSec()->GetFreq();
    } else {
      hasTempF = false;
    }
  } else {
    timeStepTemp = NULL;
  }
  
  // Initialize unit converters
  precipConvert = (3600.0 / (float)task->GetPrecipSec()->GetUnitTime()->GetTimeInSec());
  if (hasQPF) {
    qpfConvert = (3600.0 / (float)task->GetQPFSec()->GetUnitTime()->GetTimeInSec());
  }
  if (!task->GetPETSec()->IsTemperature()) {
    petConvert = (3600.0 / (float)task->GetPETSec()->GetUnitTime()->GetTimeInSec());
  } else {
    petConvert = 1.0;
  }
  timeStepHours = timeStep->GetTimeInSec() / 3600.0;

  if (timeStepLR) {
    timeStepHoursLR = timeStepLR->GetTimeInSec() / 3600.0;
    beginLRTime = *(task->GetTimeBeginLR());
  }

  // Initialize time information
  currentTime = *(task->GetTimeBegin());
  currentTimePrecip = *(task->GetTimeBegin());
  currentTimeQPF = *(task->GetTimeBegin());
  currentTimePET = *(task->GetTimeBegin());
  currentTimeTemp = *(task->GetTimeBegin());
  currentTimeTempF = *(task->GetTimeBegin());
  beginTime = *(task->GetTimeBegin());
  endTime = *(task->GetTimeEnd());
  warmEndTime = *(task->GetTimeWarmEnd());
  
  // Initialize file name information
  precipFile = task->GetPrecipSec()->GetFileName();
  if (hasQPF) {
    qpfFile = task->GetQPFSec()->GetFileName();
  }
  petFile = task->GetPETSec()->GetFileName();
  if (task->GetSnow() != SNOW_QTY) {
    tempFile = task->GetTempSec()->GetFileName();
    if (hasTempF) {
      tempFFile = task->GetTempFSec()->GetFileName();
    }
  }
  currentTimeText.SetNameStr("YYYY-MM-DD HH:UU");
  currentTimeText.ProcessNameLoose(NULL);
  currentTimeTextOutput.SetNameStr("YYYYMMDD_HHUU");
  currentTimeTextOutput.ProcessNameLoose(NULL);
  
  // Set forcing info
  precipSec = task->GetPrecipSec();
  petSec = task->GetPETSec();
  qpfSec = task->GetQPFSec();
  tempSec = task->GetTempSec();
  tempFSec = task->GetTempFSec();
  
  // Initialize our gauges
  gauges = task->GetBasinSec()->GetGauges();
  
  // Initialize parameter settings
  paramSettings = task->GetParamsSec()->GetParamSettings();
	if (task->GetRouting() != ROUTE_QTY) {
  	paramSettingsRoute = task->GetRoutingParamsSec()->GetParamSettings();
	} else {
		paramSettingsRoute = NULL;
	}
  if (task->GetSnow() != SNOW_QTY) {
    paramSettingsSnow = task->GetSnowParamsSec()->GetParamSettings();
  } else {
    paramSettingsSnow = NULL;
  }
  
  if (task->GetInundation() != INUNDATION_QTY) {
    paramSettingsInundation = task->GetInundationParamsSec()->GetParamSettings();
  } else {
    paramSettingsInundation = NULL;
  }
  
  // Initialize gridded parameter settings
  if (!InitializeGridParams(task)) {
    return false;
  }
  
  float *defaultParams = NULL, *defaultParamsRoute = NULL, *defaultParamsSnow = NULL, *defaultParamsInundation = NULL;
  GaugeConfigSection *gs = task->GetDefaultGauge();
  std::map<GaugeConfigSection *, float *>::iterator pitr = paramSettings->find(gs);
  if (pitr != paramSettings->end()) {
    defaultParams = pitr->second;
  }
  
  // Repeat for routing params
  if (task->GetRouting() != ROUTE_QTY) {
  	pitr = paramSettingsRoute->find(gs);
  	if (pitr != paramSettingsRoute->end()) {
    	defaultParamsRoute = pitr->second;
  	}
	}
  
  // Repeat for snow params
  if (task->GetSnow() != SNOW_QTY) {
    pitr = paramSettingsSnow->find(gs);
    if (pitr != paramSettingsSnow->end()) {
      defaultParamsSnow = pitr->second;
    }
  }
  
  // Repeat for inundation params
  if (task->GetInundation() != INUNDATION_QTY) {
    pitr = paramSettingsInundation->find(gs);
    if (pitr != paramSettingsInundation->end()) {
      defaultParamsInundation = pitr->second;
    }
  }
  
  // Carve the basin to find which nodes we're modeling on
  CarveBasin(task->GetBasinSec(), &nodes, paramSettings, &fullParamSettings, &gaugeMap, defaultParams, paramSettingsRoute, &fullParamSettingsRoute, defaultParamsRoute, paramSettingsSnow, &fullParamSettingsSnow, defaultParamsSnow, paramSettingsInundation, &fullParamSettingsInundation, defaultParamsInundation);
  
  // Ensure we actually have at least one node to work with!
  if (nodes.size() == 0) {
    ERROR_LOG("The number of grid cells in which we are modeling is 0! (Invalid gauge location?)");
    return false;
  }
  
  // Create the appropriate model
  switch (task->GetModel()) {
    case MODEL_CREST:
      wbModel = new CRESTModel();
      break;
    case MODEL_HYMOD:
      wbModel = new HyMOD();
      break;
    case MODEL_SAC:
      wbModel = new SAC();
      break;
    case MODEL_HP:
      wbModel = new HPModel();
      break;
    default:
      ERROR_LOG("Unsupported Water Balance Model!!");
      return false;
  }
  
  if (wbModel->IsLumped()) {
    // We need to provided updated areas
    std::vector<float> gaugeAreas;
    gaugeAreas.resize(gauges->size());
    gaugeMap.GetGaugeArea(&nodes, &gaugeAreas);
    
    lumpedNodes.resize(gauges->size());
    for (size_t i = 0; i < gauges->size(); i++) {
      GaugeConfigSection *gauge = gauges->at(i);
      memcpy(&(lumpedNodes[i]), &(nodes[gauge->GetGridNodeIndex()]), sizeof(GridNode));
      lumpedNodes[i].area = gaugeAreas[i];
    }
    rModel = NULL;
  } else {
    // Create the appropriate routing
    switch (task->GetRouting()) {
      case ROUTE_LINEAR:
        rModel = new LRRoute();
        break;
      case ROUTE_KINEMATIC:
        rModel = new KWRoute();
        break;
      case ROUTE_QTY:
				rModel = NULL;
				break;
      default:
        ERROR_LOG("Unsupported Routing Model!!");
        return false;
    }
  }
  
  // Create the appropriate snow model
  switch (task->GetSnow()) {
    case SNOW_SNOW17:
      sModel = new Snow17Model();
      break;
    case SNOW_QTY:
      sModel = NULL;
      break;
    default:
      ERROR_LOG("Unsupported Snow Model!!");
      return false;
  }
  
  switch (task->GetInundation()) {
    case INUNDATION_SI:
      iModel = new SimpleInundation();
      break;
    case INUNDATION_VCI:
      iModel = new VCInundation();
      break;
    case INUNDATION_QTY:
      iModel = NULL;
      break;
    default:
      ERROR_LOG("Unsupported inundation model!!");
      return false;
  }
  
  gaugesUsed.resize(gauges->size());
  for (size_t i = 0; i < gauges->size(); i++) {
    gaugesUsed[i] = false;
  }
  
  return true;
}

bool Simulator::InitializeSimu(TaskConfigSection *task) {
  
  char buffer[CONFIG_MAX_LEN*2];
 
   missingQPE = 0;
   missingQPF = 0;
 
  griddedOutputs = task->GetGriddedOutputs();
  useStates = task->UseStates();
  saveStates = task->SaveStates();
  
  // Initialize the storage of contributing precip & PET
  avgPrecip.resize(gauges->size());
  avgPET.resize(gauges->size());
  avgSWE.resize(gauges->size());
  avgT.resize(gauges->size());
  avgSM.resize(gauges->size());
  avgFF.resize(gauges->size());
  avgSF.resize(gauges->size());
  
  // Initialize forcing vectors & discharge storage vector
  currentPrecipSimu.resize(nodes.size());
  currentPETSimu.resize(nodes.size());
  currentTempSimu.resize(nodes.size());
  if (!wbModel->IsLumped()) {
    if (task->GetStdGrid()[0] && task->GetAvgGrid()[0] && task->GetScGrid()[0]) {
      std::vector<float> avgVals, stdVals, scVals;
      avgVals.resize(nodes.size());
      stdVals.resize(nodes.size());
      scVals.resize(nodes.size());
      if (ReadLP3File(task->GetStdGrid(), &nodes, &stdVals) && ReadLP3File(task->GetAvgGrid(), &nodes, &avgVals) && ReadLP3File(task->GetScGrid(), &nodes, &scVals)) {
        outputRP = true;
        rpData.resize(nodes.size());
        CalcLP3Vals(&stdVals, &avgVals, &scVals, &rpData, &nodes);
      } else {
        ERROR_LOGF("%s", "Failed to load LP3 grids!");
        outputRP = false;
      }
    } else {
      outputRP = false;
    }
    currentFF.resize(nodes.size());
    currentSF.resize(nodes.size());
    currentQ.resize(nodes.size());
    currentSWE.resize(nodes.size());
    currentDepth.resize(nodes.size());
  } else {
    outputRP = false;
    currentFF.resize(lumpedNodes.size());
    currentSF.resize(lumpedNodes.size());
    currentQ.resize(lumpedNodes.size());
    currentSWE.resize(lumpedNodes.size());
  }
  
  // Initialize file handles for all of the gauges we are using! Also load the time series information if appropriate.
  gaugeOutputs.resize(gauges->size());
  for (size_t i = 0; i < gauges->size(); i++) {
    gaugeOutputs[i] = NULL;
    if (gauges->at(i)->OutputTS()) {
      sprintf(buffer, "%s/ts.%s.%s.csv", task->GetOutput(), gauges->at(i)->GetName(), wbModel->GetName());
      gaugeOutputs[i] = fopen(buffer, "w");
      if (gaugeOutputs[i]) {
        //setvbuf(gaugeOutputs[i], NULL, _IONBF, 0);
        fprintf(gaugeOutputs[i], "%s", "Time,Discharge(m^3 s^-1),Observed(m^3 s^-1),Precip(mm h^-1),PET(mm h^-1),SM(%),Fast Flow(mm*1000),Slow Flow(mm*1000)");
        if (sModel) {
          fprintf(gaugeOutputs[i], "%s", ",Temperature (C),SWE(mm)");
        }
        if (outputRP) {
          fprintf(gaugeOutputs[i], "%s", ",Return Period(y)");
        }
        fprintf(gaugeOutputs[i], "%s", "\n");
        
      } else {
        WARNING_LOGF("Failed to open gauge output file \"%s\"", buffer);
      }
    }
    
    // Tell this gauge to load the observed data file
    gauges->at(i)->LoadTS();
    //   NORMAL_LOGF("%s\n", "Got here!1");
  }
  
  outputPath = task->GetOutput();
  if (useStates) {
    statePath = task->GetState();
    stateTime = *(task->GetTimeState());
  }
  
  if ((task->GetPreloadForcings())[0]) {
    totalTimeSteps = 0;
    for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
			if (timeStepLR && !inLR && beginLRTime <= currentTime) {
				inLR = true;
				timeStep = timeStepLR;
			}
      totalTimeSteps++;
    }
		inLR = false;
		timeStep = timeStepSR;
    currentPrecipCali.resize(totalTimeSteps);
    currentPETCali.resize(totalTimeSteps);
    currentTempCali.resize(totalTimeSteps);
    sprintf(buffer, "%s/%s", task->GetOutput(), task->GetPreloadForcings());
    INFO_LOGF("Preloading forcing from file %s", buffer);
    PreloadForcings(buffer, false);
    currentTime = beginTime;
    preloadedForcings = true;
  } else {
    preloadedForcings = false;
  }
  
  return true;
}

bool Simulator::InitializeCali(TaskConfigSection *task) {
  
  // Set calibration param info
  caliParamSec = task->GetCaliParamSec();
  routingCaliParamSec = task->GetRoutingCaliParamSec();
  snowCaliParamSec = task->GetSnowCaliParamSec();
  objectiveFunc = caliParamSec->GetObjFunc();
  caliGauge = caliParamSec->GetGauge();
  numWBParams = numModelParams[task->GetModel()];
	if (task->GetRouting() != ROUTE_QTY) {
  	numRParams = numRouteParams[task->GetRouting()];
	} else {
		numRParams = 0;
	}
  if (task->GetSnow() != SNOW_QTY) {
    numSParams = numSnowParams[task->GetSnow()];
  } else {
    numSParams = 0;
  }
 
	if (timeStepLR) {
		ERROR_LOGF("%s", "Long range time steps do not work in calibration mode!");
 		return false;
	}

  INFO_LOGF("Calibrating on gauge %s", caliGauge->GetName());
  
  // See if we have the approriate parameters set to do this
  if (paramSettings->find(caliGauge) == paramSettings->end()) {
    ERROR_LOGF("In order to calibrate on gauge \"%s\" it must be given parameter settings. They can not be inferred from a downstream gauge!", caliGauge->GetName());
    return false;
  }
  caliWBParams = fullParamSettings[caliGauge];
 
	if (task->GetRouting() != ROUTE_QTY) { 
  	// See if we have the approriate routing parameters set to do this
  	if (paramSettingsRoute->find(caliGauge) == paramSettingsRoute->end()) {
    	ERROR_LOGF("In order to calibrate on gauge \"%s\" it must be given routing parameter settings. They can not be inferred from a downstream gauge!", caliGauge->GetName());
    	return false;
  	}
  	caliRParams = fullParamSettingsRoute[caliGauge];
	}  

  if (task->GetSnow() != SNOW_QTY) {
    // See if we have the approriate routing parameters set to do this
    if (paramSettingsSnow->find(caliGauge) == paramSettingsSnow->end()) {
      ERROR_LOGF("In order to calibrate on gauge \"%s\" it must be given snow parameter settings. They can not be inferred from a downstream gauge!", caliGauge->GetName());
      return false;
    }
    caliSParams = fullParamSettingsSnow[caliGauge];
  }
  
  caliGauge->LoadTS();
  
  // Figure out how many time steps there are going to be
  totalTimeSteps = 0; totalTimeStepsOutsideWarm = 0;
  for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
    totalTimeSteps++;
    if (warmEndTime <= currentTime) {
      totalTimeStepsOutsideWarm++;
    }
  }
  
  // Initialize storage for forcing vectors
  currentPrecipCali.resize(totalTimeSteps);
  currentPETCali.resize(totalTimeSteps);
  if (task->GetSnow() != SNOW_QTY) {
    currentTempCali.resize(totalTimeSteps);
  }
  if (!wbModel->IsLumped()) {
    currentFF.resize(nodes.size());
    currentSF.resize(nodes.size());
    currentQ.resize(nodes.size());
    currentSWE.resize(nodes.size());
  } else {
    currentFF.resize(lumpedNodes.size());
    currentSF.resize(lumpedNodes.size());
    currentQ.resize(lumpedNodes.size());
    currentSWE.resize(lumpedNodes.size());
  }
  
  // Initialize storage for discharge vectors
  obsQ.resize(totalTimeStepsOutsideWarm);
  simQ.resize(totalTimeStepsOutsideWarm);
  
  // Get caliGaugeIndex
  for (size_t i = 0; i < gauges->size(); i++) {
    if (caliGauge == gauges->at(i)) {
      caliGaugeIndex = (int)i;
      break;
    }
  }
  
  // Initialize our parallel model sets if using OpenMP
#if _OPENMP
  int maxThreads = omp_get_max_threads();
  caliWBModels.resize(maxThreads);
  caliRModels.resize(maxThreads);
  caliSModels.resize(maxThreads);
  caliWBFullParamSettings.resize(maxThreads);
  caliWBCurrentParams.resize(maxThreads);
  caliRFullParamSettings.resize(maxThreads);
  caliRCurrentParams.resize(maxThreads);
  caliSFullParamSettings.resize(maxThreads);
  caliSCurrentParams.resize(maxThreads);
  for (int i = 0; i < maxThreads; i++) {
    switch (task->GetModel()) {
      case MODEL_CREST:
        caliWBModels[i] = new CRESTModel();
        break;
      case MODEL_HYMOD:
        caliWBModels[i] = new HyMOD();
        break;
      case MODEL_SAC:
        caliWBModels[i] = new SAC();
        break;
      case MODEL_HP:
        caliWBModels[i] = new HPModel();
        break;
      default:
        ERROR_LOG("Unsupported Model!!");
        return false;
    }
    
    // Create the appropriate routing
    switch (task->GetRouting()) {
      case ROUTE_LINEAR:
        caliRModels[i] = new LRRoute();
        break;
      case ROUTE_KINEMATIC:
        caliRModels[i] = new KWRoute();
        break;
			case ROUTE_QTY:
				caliRModels[i] = NULL;
				break;
      default:
        ERROR_LOG("Unsupported Routing Model!!");
        return false;
    }
    
    // Create the appropriate snow model
    switch (task->GetSnow()) {
      case SNOW_SNOW17:
        caliSModels[i] = new Snow17Model();
        break;
      case SNOW_QTY:
        caliSModels[i] = NULL;
        break;
      default:
        ERROR_LOG("Unsupported Snow Model!!");
        return false;
    }
    
    
    caliWBCurrentParams[i] = new float[numWBParams];
    
    for (std::map<GaugeConfigSection *, float *>::iterator itr = fullParamSettings.begin(); itr != fullParamSettings.end(); itr++) {
      if (itr->second == caliWBParams) {
        (caliWBFullParamSettings[i])[itr->first] = caliWBCurrentParams[i];
      } else {
        (caliWBFullParamSettings[i])[itr->first] = itr->second;
      }
    }
   
		if (task->GetRouting() != ROUTE_QTY) { 
			caliRCurrentParams[i] = new float[numRParams];
    	for (std::map<GaugeConfigSection *, float *>::iterator itr = fullParamSettingsRoute.begin(); itr != fullParamSettingsRoute.end(); itr++) {
     	 if (itr->second == caliRParams) {
        	(caliRFullParamSettings[i])[itr->first] = caliRCurrentParams[i];
      	} else {
        	(caliRFullParamSettings[i])[itr->first] = itr->second;
      	}
    	}
		}
    
    if (task->GetSnow() != SNOW_QTY) {
      caliSCurrentParams[i] = new float[numSParams];
      
      for (std::map<GaugeConfigSection *, float *>::iterator itr = fullParamSettingsSnow.begin(); itr != fullParamSettingsSnow.end(); itr++) {
        if (itr->second == caliSParams) {
          (caliSFullParamSettings[i])[itr->first] = caliSCurrentParams[i];
        } else {
          (caliSFullParamSettings[i])[itr->first] = itr->second;
        }
      }
    }
  }
#endif
  
  return true;
}

void Simulator::CleanUp() {
  // Close output gauge files
  for (size_t i = 0; i < gaugeOutputs.size(); i++) {
    if (gaugeOutputs[i]) {
      fclose(gaugeOutputs[i]);
    }
  }
}

void Simulator::BasinAvg() {
  PrecipReader precipReader;
  char buffer[CONFIG_MAX_LEN*2];
#if _OPENMP
  double timeTotal = 0.0, timeCount = 0.0;
#endif
  
  std::vector<float> avgVals;
  long numNodes = nodes.size();
  avgVals.resize(numNodes);
  
  gridWriter.Initialize();
  
  // This is the temporal loop for each time step
  // Here we load the input forcings & actually run the model
  for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
#if _OPENMP
#ifndef _WIN32
    double beginTime = omp_get_wtime();
#endif
#endif
    currentTimeText.UpdateName(currentTime.GetTM());
#ifndef _WIN32
    NORMAL_LOGF("%s", currentTimeText.GetName());
#else
    setTimestep(currentTimeText.GetName());
#endif
    
    currentTimeTextOutput.UpdateName(currentTime.GetTM());
    
    LoadForcings(&precipReader, NULL, NULL);
    
    for (long i = numNodes - 1; i >= 0; i--) {
      GridNode *node = &(nodes[i]);
      float addVal = avgVals[i] + currentPrecipSimu[i];
      avgVals[i] = addVal / nodes[i].contribArea;
      if (node->downStreamNode != INVALID_DOWNSTREAM_NODE) {
        avgVals[node->downStreamNode] += addVal;
      }
    }
    
    sprintf(buffer, "%s/precip.%s.avg.tif", outputPath, currentTimeTextOutput.GetName());
    gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
    
    for (long i = numNodes - 1; i >= 0; i--) {
      avgVals[i] = 0.0;
    }
    
#if _OPENMP
#ifndef _WIN32
    double endTime = omp_get_wtime();
    double timeDiff = endTime - beginTime;
    NORMAL_LOGF(" %f sec", endTime - beginTime);
    timeTotal += timeDiff;
    timeCount++;
    if (timeCount == 250) {
      NORMAL_LOGF(" (%f sec avg)", timeTotal / timeCount);
      timeCount = 0.0;
      timeTotal = 0.0;
    }
#endif
#endif
    
    // All of our status messages are done for this timestep!
#ifndef _WIN32
    NORMAL_LOGF("%s", "\n");
#endif
    
  }
  
  for (long i = numNodes - 1; i >= 0; i--) {
    float areaUsed = (nodes[i].contribArea > 100000.0) ? 100000.0 : nodes[i].contribArea;
    areaUsed = (areaUsed < 3.0) ? 3.0 : areaUsed;
    avgVals[i] = 0.000503442*powf(areaUsed, -0.47)*powf(currentPrecipSimu[i],1.25)*nodes[i].contribArea;
		avgVals[i] = 0.3012*powf(avgVals[i],1.1894);
    //avgVals[i] = 8.24*powf(areaUsed, -0.57)*nodes[i].contribArea;
  }
  sprintf(buffer, "%s/actionFloodThresPrecip.tif", outputPath);
  gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
  
  for (long i = numNodes - 1; i >= 0; i--) {
    float areaUsed = (nodes[i].contribArea > 100000.0) ? 100000.0 : nodes[i].contribArea;
    areaUsed = (areaUsed < 3.0) ? 3.0 : areaUsed;
    avgVals[i] = 0.00078398*powf(areaUsed, -0.47)*powf(currentPrecipSimu[i],1.25)*nodes[i].contribArea;
		avgVals[i] = 0.3012*powf(avgVals[i],1.1894);
    //avgVals[i] = 8.24*powf(areaUsed, -0.57)*nodes[i].contribArea;
  }
  sprintf(buffer, "%s/minorFloodThresPrecip.tif", outputPath);
  gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
  
  for (long i = numNodes - 1; i >= 0; i--) {
    float areaUsed = (nodes[i].contribArea > 100000.0) ? 100000.0 : nodes[i].contribArea;
    areaUsed = (areaUsed < 3.0) ? 3.0 : areaUsed;
    avgVals[i] = 0.001308855*powf(areaUsed, -0.47)*powf(currentPrecipSimu[i],1.25)*nodes[i].contribArea;
		avgVals[i] = 0.3012*powf(avgVals[i],1.1894);
    //avgVals[i] = 8.24*powf(areaUsed, -0.57)*nodes[i].contribArea;
  }
  sprintf(buffer, "%s/moderateFloodThresPrecip.tif", outputPath);
  gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
  
  for (long i = numNodes - 1; i >= 0; i--) {
    float areaUsed = (nodes[i].contribArea > 100000.0) ? 100000.0 : nodes[i].contribArea;
    areaUsed = (areaUsed < 3.0) ? 3.0 : areaUsed;
    avgVals[i] = 0.001995269*powf(areaUsed, -0.47)*powf(currentPrecipSimu[i],1.25)*nodes[i].contribArea;
		avgVals[i] = 0.3012*powf(avgVals[i],1.1894);
    //avgVals[i] = 8.24*powf(areaUsed, -0.57)*nodes[i].contribArea;
  }
  sprintf(buffer, "%s/majorFloodThresPrecip.tif", outputPath);
  gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
  
  for (long i = numNodes - 1; i >= 0; i--) {
    float areaUsed = (nodes[i].contribArea > 100000.0) ? 100000.0 : nodes[i].contribArea;
    areaUsed = (areaUsed < 3.0) ? 3.0 : areaUsed;
    avgVals[i] = 8.502339237*powf(areaUsed, -0.57)*nodes[i].contribArea;
  }
  sprintf(buffer, "%s/minorFloodThres.tif", outputPath);
  gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
  
  /*std::vector<float> rpavgVals, stdVals, scVals;
   rpavgVals.resize(nodes.size());
   stdVals.resize(nodes.size());
   scVals.resize(nodes.size());
   if (ReadLP3File(task->GetStdGrid(), &nodes, &stdVals) && ReadLP3File(task->GetAvgGrid(), &nodes, &rpavgVals) && ReadLP3File(task->GetScGrid(), &nodes, &scVals)) {
   rpData.resize(nodes.size());
   CalcLP3Vals(&stdVals, &rpavgVals, &scVals, &rpData, &nodes);
   for (long i = numNodes - 1; i >= 0; i--) {
   avgVals[i] = rpData[i].q5;
   }
   sprintf(buffer, "%s/q_5.tif", outputPath);
   gridWriter.WriteGrid(&nodes, &avgVals, buffer, false);
   }*/
  
}

void Simulator::Simulate(bool trackPeaks) {
  
  if (!wbModel->IsLumped()) {
    SimulateDistributed(trackPeaks);
  } else {
    SimulateLumped();
  }
  
}

float Simulator::GetNumSimulatedYears() {
  int currentYear = -1;
  float numYears = 0;
  TimeVar tempTime = warmEndTime;
  for (tempTime.Increment(timeStep); tempTime <= endTime; tempTime.Increment(timeStep)) {
    if (tempTime.GetTM()->tm_year != currentYear) {
      numYears++;
      currentYear = tempTime.GetTM()->tm_year;
    }
  }
  return numYears;
}

int Simulator::LoadForcings(PrecipReader *precipReader, PETReader *petReader, TempReader *tempReader) {
  char buffer[CONFIG_MAX_LEN*2], qpfBuffer[CONFIG_MAX_LEN*2];
  int retVal = 0;
#ifdef _WIN32
  bool outputError = false;
#endif
  if (currentTimePrecip < currentTime) {
    currentTimePrecip.Increment(timeStepPrecip);
    precipFile->UpdateName(currentTimePrecip.GetTM());
  }
  
  if (hasQPF && currentTimeQPF < currentTime) {
    currentTimeQPF.Increment(timeStepQPF);
    qpfFile->UpdateName(currentTimeQPF.GetTM());
  }
  
  if (currentTimePET < currentTime) {
    currentTimePET.Increment(timeStepPET);
    petFile->UpdateName(currentTimePET.GetTM());
  }
  
  if (sModel && tempReader) {
    if (currentTimeTemp < currentTime) {
      currentTimeTemp.Increment(timeStepTemp);
      tempFile->UpdateName(currentTimeTemp.GetTM());
    }
    
    if (hasTempF && currentTimeTempF < currentTime) {
      currentTimeTempF.Increment(timeStepTempF);
      tempFFile->UpdateName(currentTimeTempF.GetTM());
    }
    
    sprintf(buffer, "%s/%s", tempSec->GetLoc(), tempFile->GetName());
    if (!tempReader->Read(buffer, tempSec->GetType(), &nodes, &currentTempSimu, NULL, hasTempF)) {
      if (hasTempF) {
        sprintf(qpfBuffer, "%s/%s", tempFSec->GetLoc(), tempFFile->GetName());
      }
      if (!hasTempF || !tempReader->Read(qpfBuffer, tempSec->GetType(), &nodes, &currentTempSimu, NULL, false)) {
#ifdef _WIN32
        outputError = true;
#endif
        NORMAL_LOGF(" Missing Temp file(%s%s%s)... Assuming zeros.", buffer, (!hasTempF)?"":"; ", (!hasTempF)?"":qpfBuffer);
      }
    }
  }
  
  if (precipReader) {
    sprintf(buffer, "%s/%s", precipSec->GetLoc(), precipFile->GetName());
    if (!precipReader->Read(buffer, precipSec->GetType(), &nodes, &currentPrecipSimu, precipConvert, NULL, hasQPF)) {
      if (hasQPF) {
        sprintf(qpfBuffer, "%s/%s", qpfSec->GetLoc(), qpfFile->GetName());
      }
      if (!hasQPF || !precipReader->Read(qpfBuffer, qpfSec->GetType(), &nodes, &currentPrecipSimu, qpfConvert, NULL, false)) {
#ifdef _WIN32
        outputError = true;
#endif
        NORMAL_LOGF(" Missing precip file(%s%s%s)... Assuming zeros.", buffer, (!hasQPF)?"":"; ", (!hasQPF)?"":qpfBuffer);
	if (inLR) {
		missingQPF = missingQPF + 1;
	} else {
		missingQPE = missingQPE + 1;
	}	
      } else if (hasQPF) {
        retVal = 1;
      }
    }
  }
  
  if (petReader) {
    sprintf(buffer, "%s/%s", petSec->GetLoc(), petFile->GetName());
    if (!petReader->Read(buffer, petSec->GetType(), &nodes, &currentPETSimu, petConvert, petSec->IsTemperature(), (float)currentTime.GetTM()->tm_yday)) {
#ifdef _WIN32
      outputError = true;
#endif
      NORMAL_LOGF(" Missing PET file(%s)... Assuming zeros.", buffer);
    }
#ifdef _WIN32
    if (outputError) {
      NORMAL_LOGF("%s", "\n");
    }
#endif
  }
  
  return retVal;
}

void Simulator::SaveLP3Params() {
  char buffer[CONFIG_MAX_LEN*2];
  std::vector<float> avgGrid, stdGrid, csGrid;
  avgGrid.resize(currentFF.size());
  stdGrid.resize(currentFF.size());
  csGrid.resize(currentFF.size());
  // Convert to log
  for (size_t i = 0; i < currentFF.size(); i++) {
    for (int j = 0; j < numYears; j++) {
      if (peakVals[j][i] == 0.0) {
        peakVals[j][i] = 0.0000001;
      }
      peakVals[j][i] = log10(peakVals[j][i]);
    }
  }
  
  // Calculate average
  for (size_t i = 0; i < currentFF.size(); i++) {
    for (int j = 0; j < numYears; j++) {
      avgGrid[i] += peakVals[j][i];
    }
    avgGrid[i] /= numYears;
  }
  
  // Calculate standard deviation
  for (size_t i = 0; i < currentFF.size(); i++) {
    for (int j = 0; j < numYears; j++) {
      stdGrid[i] += powf(peakVals[j][i] - avgGrid[i], 2.0);
    }
    stdGrid[i] /= (numYears - 1.0);
    stdGrid[i] = sqrt(stdGrid[i]);
  }
  
  // Calculate the skewness coefficient
  for (size_t i = 0; i < currentFF.size(); i++) {
    float total = 0.0;
    for (int j = 0; j < numYears; j++) {
      total += powf(peakVals[j][i] - avgGrid[i], 3.0);
    }
    float csNum = numYears*total;
    float csDom = (numYears - 1.0) * (numYears - 2.0) * powf(stdGrid[i], 3.0);
    csGrid[i] = csNum / csDom;
  }
  
  sprintf(buffer, "%s/avgq.%s.tif", outputPath, wbModel->GetName());
  gridWriter.WriteGrid(&nodes, &avgGrid, buffer, false);
  
  sprintf(buffer, "%s/stdq.%s.tif", outputPath, wbModel->GetName());
  gridWriter.WriteGrid(&nodes, &stdGrid, buffer, false);
  
  sprintf(buffer, "%s/sc.%s.tif", outputPath, wbModel->GetName());
  gridWriter.WriteGrid(&nodes, &csGrid, buffer, false);
}

void Simulator::SaveTSOutput() {
  for (size_t i = 0; i < gauges->size(); i++) {
    GaugeConfigSection *gauge = gauges->at(i);
    if (gaugeOutputs[i]) {
      fprintf(gaugeOutputs[i], "%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f", currentTimeText.GetName(), currentQ[gauge->GetGridNodeIndex()], gauge->GetObserved(&currentTime), avgPrecip[i], avgPET[i], avgSM[i], avgFF[i]*1000.0, avgSF[i]*1000.0);
      if (sModel) {
        fprintf(gaugeOutputs[i], ",%.2f,%.2f", avgT[i], avgSWE[i]);
      }
      if (outputRP) {
        fprintf(gaugeOutputs[i], ",%.2f", GetReturnPeriod(currentQ[gauge->GetGridNodeIndex()], &(rpData[gauge->GetGridNodeIndex()])));
      }
      fprintf(gaugeOutputs[i], "%s", "\n");
    }
  }
}

bool Simulator::IsOutputTS() {
  bool wantoutput = false;
  for (size_t i = 0; i < gauges->size(); i++) {
    if (gaugeOutputs[i]) {
      wantoutput = true;
      break;
    }
  }
  if (!wantoutput) {
    INFO_LOGF("%s", "No time series are being output!");
  }
  return wantoutput;
}

void Simulator::LoadDAFile(TaskConfigSection *task) {
  wantsDA = false;
  if ((task->GetDAFile())[0]) {
    
    FILE *tsFile = fopen(task->GetDAFile(), "rb");
    if (tsFile == NULL) {
      WARNING_LOGF("Failed to open data assimilation file %s", task->GetDAFile());
      return;
    }
    wantsDA = true;
    //Get number of file lines
    int fileLines = 0, temp;
    while ( (temp = fgetc(tsFile)) != EOF) {
      fileLines += (temp == 10);
    }
    fseek(tsFile, 0, SEEK_SET);
    
    for (int i = 0; i < fileLines; i++) {
      char bufferGauge[CONFIG_MAX_LEN], bufferTime[CONFIG_MAX_LEN];
      float dataValue;
      if (fscanf(tsFile, "%[^,],%[^,],%f%*c", &(bufferGauge[0]), &(bufferTime[0]), &dataValue) == 3) {
        for (size_t i = 0; i < gauges->size(); i++) {
          if (!strcasecmp(gauges->at(i)->GetName(), bufferGauge)) {
            gauges->at(i)->SetObservedValue(bufferTime, dataValue);
            break;
          }
        }
      } else {
        char *output = fgets(bufferGauge, CONFIG_MAX_LEN, tsFile);
	(void)output;
      }
    }
    
    fclose(tsFile);
  }
}

void Simulator::AssimilateData() {
  char buffer[254];
  sprintf(buffer, "%s/da_log.csv", task->GetOutput());
  FILE *fp = fopen(buffer, "a");
  for (size_t i = 0; i < gauges->size(); i++) {
    GaugeConfigSection *gauge = gauges->at(i);
    if (!gauge->WantDA()) {
      continue;
    }
    float obs = gauge->GetObserved(&currentTime, 3600.0);
    if (obs == obs && obs > 0.0) {
      float oldValue = rModel->SetObsInflow(gauge->GetGridNodeIndex(), obs);
      //if (!gaugesUsed[i]) {
      fprintf(fp, "%s,%s,%f,%f\n", gauge->GetName(), currentTimeText.GetName(), oldValue, obs);
      gaugesUsed[i] = true;
      //}
    }
  }
  fclose(fp);
}

void Simulator::OutputCombinedOutput() {
  if (!task->GetCOFile()[0]) {
    return;
  }
  FILE *fp = fopen(task->GetCOFile(), "a");
  for (size_t i = 0; i < gauges->size(); i++) {
    GaugeConfigSection *gauge = gauges->at(i);
    if (!gauge->WantCO()) {
      continue;
    }
    fprintf(fp, "%s,%s,%f\n", currentTimeText.GetName(), gauge->GetName(), currentQ[gauge->GetGridNodeIndex()]);
  }
  fclose(fp);
}

bool Simulator::ReadThresFile(char *file, std::vector<GridNode> *nodes, std::vector<float> *thresVals) {
  FloatGrid *grid = NULL;
  
  grid = ReadFloatTifGrid(file);
  
  if (!grid) {
    WARNING_LOGF("Failed to open threshold file %s", file);
    return false;
  }
  
  // We have two options now... Either the grid & the basic grids are the same
  // Or they are different!
  
  if (g_DEM->IsSpatialMatch(grid)) {
    INFO_LOGF("Loading exact match threshold grid %s", file);
    // The grids are the same! Our life is easy!
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (grid->data[node->y][node->x] != grid->noData) {
        thresVals->at(i) = grid->data[node->y][node->x];
      } else {
        thresVals->at(i) = 0;
      }
    }
    
  } else {
    INFO_LOGF("Threshold grids aren't an exact match so guessing! %s", file);
    // The grids are different, we must do some resampling fun.
    GridLoc pt;
    for (size_t i = 0; i < nodes->size(); i++) {
      GridNode *node = &(nodes->at(i));
      if (grid->GetGridLoc(node->refLoc.x, node->refLoc.y, &pt) && grid->data[pt.y][pt.x] != grid->noData) {
        thresVals->at(i) = grid->data[pt.y][pt.x];
      } else {
        thresVals->at(i) = 0;
      }
    }
    
  }
  
  delete grid;
  
  return true;
}

float Simulator::ComputeThresValue(float discharge, float action, float minor, float moderate, float major) {
  float result = g_DEM->noData;
  if (major == major && discharge >= major) {
    result = (discharge-major)/major + 4.0;
  } else if (major == major && moderate == moderate && discharge >= moderate) {
    result = (discharge-moderate)/(major - moderate) + 3.0;
  } else if (moderate == moderate && minor == minor && discharge >= minor) {
    result = (discharge-minor)/(moderate-minor) + 2.0;
  } else if (minor == minor && action == action && discharge >= action) {
    result = (discharge-action)/(minor-action) + 1.0;
  } else if (action == action) {
    result = discharge/action;
  }
  
  if (!std::isfinite(result) || result > 10.0) {
    result = g_DEM->noData;
  }
  
  return result;
}

float Simulator::ComputeThresValueP(float discharge, float action, float actionSD, float minor, float minorSD, float moderate, float moderateSD, float major, float majorSD) {
  float result = g_DEM->noData;
  float thres = 0.8;
  if (major == major && CalcProb(discharge, major, majorSD) > thres) {
    result = CalcProb(discharge, major, majorSD) + 4.0;
  } else if (major == major && moderate == moderate && CalcProb(discharge, moderate, moderateSD) > thres) {
    result = CalcProb(discharge, major, majorSD) + 3.0;
  } else if (moderate == moderate && minor == minor && CalcProb(discharge, minor, minorSD) > thres) {
    result = CalcProb(discharge, moderate, moderateSD) + 2.0;
  } else if (minor == minor && action == action && CalcProb(discharge, action, actionSD) > thres) {
    result = CalcProb(discharge, minor, minorSD) + 1.0;
  } else if (action == action) {
    result = CalcProb(discharge, action, actionSD);
  }
  
  if (!std::isfinite(result) || result > 10.0) {
    result = g_DEM->noData;
  }
  
  return result;
}

float Simulator::CalcProb(float discharge, float mean, float sd) {
  return 0.5f * (1.0 + erf((discharge - mean) / (logf(sd) * sqrtf(2))));
}

void Simulator::SimulateDistributed(bool trackPeaks) {
  PrecipReader precipReader;
  PETReader petReader;
  TempReader tempReader;
  std::vector<float> currentPrecipSnow;
  std::vector<float> *currentPrecip = &currentPrecipSimu;
  char buffer[CONFIG_MAX_LEN*2];
  size_t tsIndex = 0;
  bool outputTS = IsOutputTS();
  //NORMAL_LOGF("%s\n", "Got here!3");
  // Peak tracking variables
  numYears = 0;
  int currentYear = -1;
  int indexYear = -1;
  
  // Initialize TempReader
  if (sModel) {
    tempReader.ReadDEM(tempSec->GetDEM());
  } else {
    tempReader.SetNullDEM();
  }
  
  std::vector<float> actionVals, minorVals, moderateVals, majorVals;
  std::vector<float> actionSDVals, minorSDVals, moderateSDVals, majorSDVals;
  bool outputThres = false, outputThresP = false;
  
  if (((griddedOutputs & OG_THRES) == OG_THRES || (griddedOutputs & OG_MAXTHRES) == OG_MAXTHRES) && (task->GetActionGrid())[0] && (task->GetMinorGrid())[0] && (task->GetModerateGrid())[0] && (task->GetMajorGrid())[0]) {
    actionVals.resize(nodes.size());
    minorVals.resize(nodes.size());
    moderateVals.resize(nodes.size());
    majorVals.resize(nodes.size());
    if (ReadThresFile(task->GetActionGrid(), &nodes, &actionVals) && ReadThresFile(task->GetMinorGrid(), &nodes, &minorVals) && ReadThresFile(task->GetModerateGrid(), &nodes, &moderateVals) && ReadThresFile(task->GetMajorGrid(), &nodes, &majorVals)) {
      outputThres = true;
      
      if (((griddedOutputs & OG_MAXTHRESP) == OG_MAXTHRESP) && (task->GetActionSDGrid())[0] && (task->GetMinorSDGrid())[0] && (task->GetModerateSDGrid())[0] && (task->GetMajorSDGrid())[0]) {
        actionSDVals.resize(nodes.size());
        minorSDVals.resize(nodes.size());
        moderateSDVals.resize(nodes.size());
        majorSDVals.resize(nodes.size());
        if (ReadThresFile(task->GetActionSDGrid(), &nodes, &actionSDVals) && ReadThresFile(task->GetMinorSDGrid(), &nodes, &minorSDVals) && ReadThresFile(task->GetModerateSDGrid(), &nodes, &moderateSDVals) && ReadThresFile(task->GetMajorSDGrid(), &nodes, &majorSDVals)) {
          outputThresP = true;
          for (size_t i = 0; i < currentFF.size(); i++) {
            actionSDVals[i] = 3.67 * nodes[i].contribArea;
            minorSDVals[i] = 3.67 * nodes[i].contribArea;
            moderateSDVals[i] = 3.67 * nodes[i].contribArea;
            majorSDVals[i] = 3.67 * nodes[i].contribArea;
            
            
          }
        }
        
      }
      
    }
    
  }
  
  bool savePrecip = false;
  std::vector<float> qpeAccum, qpfAccum;
  if ((griddedOutputs & OG_PRECIPACCUM) == OG_PRECIPACCUM) {
    qpeAccum.resize(nodes.size());
    qpfAccum.resize(nodes.size());
    savePrecip = true;
  }
  
  // Initialize our models
  // NORMAL_LOGF("%s\n", "Got here!4");
  wbModel->InitializeModel(&nodes, &fullParamSettings, &paramGrids);
  //	NORMAL_LOGF("%s\n", "Got here!5");
  if (rModel) {
  	rModel->InitializeModel(&nodes, &fullParamSettingsRoute, &paramGridsRoute);
	}
  //	NORMAL_LOGF("%s\n", "Got here!6");
  if (sModel) {
    currentPrecipSnow.resize(currentPrecipSimu.size());
    sModel->InitializeModel(&nodes, &fullParamSettingsSnow, &paramGridsSnow);
  }
  if (iModel) {
    iModel->InitializeModel(&nodes, &fullParamSettingsInundation, &paramGridsInundation);
  }
  if (griddedOutputs != OG_NONE || trackPeaks || outputRP || saveStates) {
    gridWriter.Initialize();
  }
  if (useStates) {
    wbModel->InitializeStates(&currentTime, statePath);
		if (rModel) {
    	rModel->InitializeStates(&currentTime, statePath, &currentFF, &currentSF);
		}
    if (sModel) {
      sModel->InitializeStates(&currentTime, statePath);
    }
  } else {
    for (size_t i = 0; i < currentFF.size(); i++) {
      currentFF[i] = 0.0;
      currentSF[i] = 0.0;
      currentSWE[i] = 0.0;
    }
  }
  
  // Set up stuff for peak tracking here, if needed
  if (trackPeaks) {
    numYears = GetNumSimulatedYears();
    INFO_LOGF("Number of years is %f\n", numYears);
    
    // Allocate storage
    peakVals.resize((int)numYears);
    for (int i = 0; i < numYears; i++) {
      peakVals[i].resize(currentFF.size());
    }
  }
  
  // Hard coded RP counting
  std::vector<float> count2, rpGrid, rpMaxGrid, maxGrid;
  std::vector<float> SM;
  std::vector<float> dailyMaxQ, dailyMinSM, dailyMaxQHour;
  count2.resize(currentFF.size());
  rpGrid.resize(currentFF.size());
  rpMaxGrid.resize(currentFF.size());
  maxGrid.resize(currentFF.size());
  SM.resize(currentFF.size());
  
  for (size_t i = 0; i < currentFF.size(); i++) {
    count2[i] = 0.0;
    rpGrid[i] = 0.0;
    rpMaxGrid[i] = 0.0;
    maxGrid[i] = 0.0;
    currentFF[i] = 0.0;
    currentSF[i] = 0.0;
    currentQ[i] = 0.0;
  }
  
#if _OPENMP
  double timeTotal = 0.0, timeCount = 0.0;
  double simStartTime = omp_get_wtime();
#endif
  
  // This is the temporal loop for each time step
  // Here we load the input forcings & actually run the model
  for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
#if _OPENMP
#ifndef _WIN32
    double beginTime = omp_get_wtime();
#endif
#endif
    currentTimeText.UpdateName(currentTime.GetTM());
#ifndef _WIN32
    NORMAL_LOGF("%s", currentTimeText.GetName());
#else
    setTimestep(currentTimeText.GetName());
#endif
    
    int qpf = 0;
    if (!preloadedForcings) {
      qpf = LoadForcings(&precipReader, &petReader, &tempReader);
      currentPrecip = &currentPrecipSimu;
    }
    
    float stepHoursReal = timeStep->GetTimeInSec() / 3600.0f;
    
    if (sModel) {
      if (preloadedForcings) {
        sModel->SnowBalance((float)currentTime.GetTM()->tm_yday, stepHoursReal, &(currentPrecipCali[tsIndex]), &(currentTempCali[tsIndex]), &(currentPrecipCali[tsIndex]), &currentSWE);
      } else {
        sModel->SnowBalance((float)currentTime.GetTM()->tm_yday, stepHoursReal, currentPrecip, &currentTempSimu, &currentPrecipSnow, &currentSWE);
        currentPrecip = &currentPrecipSnow;
      }
    }
    
    // Integrate the models for this timestep
    if (!preloadedForcings) {
      wbModel->WaterBalance(stepHoursReal, currentPrecip, &currentPETSimu, &currentFF, &currentSF, &SM);
    } else {
      wbModel->WaterBalance(stepHoursReal, &(currentPrecipCali[tsIndex]), &(currentPETCali[tsIndex]), &currentFF, &currentSF, &SM);
    }
    if (outputTS) {
      gaugeMap.GaugeAverage(&nodes, &currentFF, &avgFF);
      gaugeMap.GaugeAverage(&nodes, &currentSF, &avgSF);
    }
    
    if (rModel && wantsDA) {
      AssimilateData();
    }
		if (rModel) {
#if _OPENMP
#ifndef _WIN32
    	double beginTimeR = omp_get_wtime(); 
#endif
#endif
    	rModel->Route(stepHoursReal, &currentFF, &currentSF, &currentQ);
#if _OPENMP
#ifndef _WIN32
    	double endTimeR = omp_get_wtime();
    	NORMAL_LOGF(" %f routing sec", endTimeR - beginTimeR); 
#endif
#endif
		} else {
			for (size_t i = 0; i < currentFF.size(); i++) {
				currentFF[i] = 0.0;
				currentSF[i] = 0.0;
			}	
		}
    if (saveStates && stateTime == currentTime) {
      wbModel->SaveStates(&currentTime, statePath, &gridWriter);
			if (rModel) {
      	rModel->SaveStates(&currentTime, statePath, &gridWriter);
			}
      if (sModel) {
        sModel->SaveStates(&currentTime, statePath, &gridWriter);
      }
    }
    
    // We only output after the warmup period is over
    if (warmEndTime <= currentTime) {
      
      if (savePrecip) {
        for (size_t i = 0; i < currentFF.size(); i++) {
          float precip = currentPrecip->at(i) * stepHoursReal;
          if (qpf) {
            qpfAccum[i] += precip;
          } else {
            qpeAccum[i] += precip;
          }
        }
      }
      
      OutputCombinedOutput();
      
      if (trackPeaks && currentYear != currentTime.GetTM()->tm_year) {
        currentYear = currentTime.GetTM()->tm_year;
        indexYear++;
      }
      
      if (outputTS) {
        gaugeMap.GaugeAverage(&nodes, &SM, &avgSM);
        if (!preloadedForcings) {
          gaugeMap.GaugeAverage(&nodes, currentPrecip, &avgPrecip);
          gaugeMap.GaugeAverage(&nodes, &currentPETSimu, &avgPET);
        } else {
          gaugeMap.GaugeAverage(&nodes, &(currentPrecipCali[tsIndex]), &avgPrecip);
          gaugeMap.GaugeAverage(&nodes, &(currentPETCali[tsIndex]), &avgPET);
        }
        
        if (sModel) {
          gaugeMap.GaugeAverage(&nodes, &currentSWE, &avgSWE);
          if (!preloadedForcings) {
            gaugeMap.GaugeAverage(&nodes, &currentTempSimu, &avgT);
          } else {
            gaugeMap.GaugeAverage(&nodes, &(currentTempCali[tsIndex]), &avgT);
          }
        }
        
        
        // Write the output to file
        SaveTSOutput();
      }
      
      if (trackPeaks) {
        for (size_t i = 0; i < currentFF.size(); i++) {
          if (currentQ[i] > peakVals[indexYear][i]) {
            peakVals[indexYear][i] = currentQ[i];
          }
        }
      }
      
      // Hard coded rp counting
      for (size_t i = 0; i < currentFF.size(); i++) {
        float discharge = currentQ[i];
        if (discharge > maxGrid[i]) {
          maxGrid[i] = discharge;
        }
        if (outputRP) {
          rpGrid[i] = GetReturnPeriod(discharge, &(rpData[i]));
          if (rpGrid[i] > rpMaxGrid[i]) {
            rpMaxGrid[i] = rpGrid[i];
          }
        }
      }
      
      /*int month = currentTime.GetTM()->tm_mon;
       for (size_t i = 0; i < currentFF.size(); i++) {
       if (currentQ[i] > monthlyMaxGrid[month][i]) {
       monthlyMaxGrid[month][i] = currentQ[i];
       }
       }*/
      
      if (griddedOutputs != OG_NONE) {
        currentTimeTextOutput.UpdateName(currentTime.GetTM());
      }
      
      if ((griddedOutputs & OG_Q) == OG_Q) {
        sprintf(buffer, "%s/q.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
	for (size_t i = 0; i < currentQ.size(); i++) {
        	float val = floorf(currentQ[i] * 10.0f + 0.5f) / 10.0f;
        	currentDepth[i] = val;
    	}
        gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
      }
      if ((griddedOutputs & OG_SM) == OG_SM) {
        sprintf(buffer, "%s/sm.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &SM, buffer, false);
      }
      if (outputRP && ((griddedOutputs & OG_QRP) == OG_QRP)) {
        sprintf(buffer, "%s/rp.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &rpGrid, buffer, false);
      }
      if ((griddedOutputs & OG_PRECIP) == OG_PRECIP) {
        sprintf(buffer, "%s/precip.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentPrecipSimu, buffer, false);
      }
      if ((griddedOutputs & OG_PET) == OG_PET) {
        sprintf(buffer, "%s/pet.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentPETSimu, buffer, false);
      }
      if (sModel && (griddedOutputs & OG_SWE) == OG_SWE) {
        sprintf(buffer, "%s/swe.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentSWE, buffer, false);
      }
      if (sModel && (griddedOutputs & OG_TEMP) == OG_TEMP) {
        sprintf(buffer, "%s/temp.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentTempSimu, buffer, false);
      }
      if (iModel && (griddedOutputs & OG_DEPTH) == OG_DEPTH) {
        iModel->Inundation(&currentQ, &currentDepth);
        sprintf(buffer, "%s/depth.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), iModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
      }
      if ((griddedOutputs & OG_UNITQ) == OG_UNITQ) {
        for (size_t i = 0; i < currentQ.size(); i++) {
          currentDepth[i] = currentQ[i] / nodes[i].contribArea;
	  float val = floorf(currentDepth[i] * 10.0f + 0.5f) / 10.0f;
      	  currentDepth[i] = val;
        }
        sprintf(buffer, "%s/unitq.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
      }
      if (outputThres && (griddedOutputs & OG_THRES) == OG_THRES) {
        for (size_t i = 0; i < currentQ.size(); i++) {
          currentDepth[i] = ComputeThresValue(currentQ[i], actionVals[i], minorVals[i], moderateVals[i], majorVals[i]);
        }
        sprintf(buffer, "%s/thres.%s.%s.tif", outputPath, currentTimeTextOutput.GetName(), wbModel->GetName());
        gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
        
      }
      
      
    }

#if _OPENMP
#ifndef _WIN32
    double endTime = omp_get_wtime();
    double timeDiff = endTime - beginTime;
    NORMAL_LOGF(" %f sec", endTime - beginTime);
    timeTotal += timeDiff;
    timeCount++;
    if (timeCount == 250) {
      NORMAL_LOGF(" (%f sec avg)", timeTotal / timeCount);
      timeCount = 0.0;
      timeTotal = 0.0;
    }
#endif
#endif

		if (timeStepLR && !inLR && beginLRTime <= currentTime) {
			inLR = true;
			timeStep = timeStepLR;
			NORMAL_LOGF(" Switching to long range timestep %f hours", timeStepHoursLR);
    }
    
    // All of our status messages are done for this timestep!
#ifndef _WIN32
    NORMAL_LOGF("%s", "\n");
#endif
    tsIndex++;
  }
  
  if (trackPeaks) {
    SaveLP3Params();
  }
  
  tm *ctWE = warmEndTime.GetTM();
  
  // Hard coded event counting
  if (outputRP && ((griddedOutputs & OG_MAXQRP) == OG_MAXQRP)) {
    
    sprintf(buffer, "%s/maxrp.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    for (size_t i = 0; i < currentQ.size(); i++) {
        float val = floorf(rpMaxGrid[i] + 0.5f);
        rpMaxGrid[i] = val;
    }
    gridWriter.WriteGrid(&nodes, &rpMaxGrid, buffer, false);
    
    //sprintf(buffer, "%s/qmax.%s.asc", outputPath, model->GetName());
    //gridWriter.WriteGrid(&nodes, &maxGrid, buffer);
  }
  
  if ((griddedOutputs & OG_MAXSM) == OG_MAXSM) {
    sprintf(buffer, "%s/maxsm.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    for (size_t i = 0; i < currentQ.size(); i++) {
        float val = floorf(SM[i] + 0.5f);
        SM[i] = val;
    }
    gridWriter.WriteGrid(&nodes, &SM, buffer, false);
  }
  
  if ((griddedOutputs & OG_MAXQ) == OG_MAXQ) {
    sprintf(buffer, "%s/maxq.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    for (size_t i = 0; i < currentQ.size(); i++) {
    	float val = floorf(maxGrid[i] * 10.0f + 0.5f) / 10.0f;
	currentDepth[i] = val;
    }
    gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
  }
  
  if (sModel && (griddedOutputs & OG_MAXSWE) == OG_MAXSWE) {
    sprintf(buffer, "%s/maxswe.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    gridWriter.WriteGrid(&nodes, &currentSWE, buffer, false);
  }
  
  if ((griddedOutputs & OG_MAXUNITQ) == OG_MAXUNITQ) {
    for (size_t i = 0; i < currentQ.size(); i++) {
      currentDepth[i] = maxGrid[i] / nodes[i].contribArea;
      float val = floorf(currentDepth[i] * 10.0f + 0.5f) / 10.0f;
      currentDepth[i] = val;
    }
    sprintf(buffer, "%s/maxunitq.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
  }
  
  if (outputThres && (griddedOutputs & OG_MAXTHRES) == OG_MAXTHRES) {
    for (size_t i = 0; i < currentQ.size(); i++) {
      currentDepth[i] = floorf(ComputeThresValue(maxGrid[i], actionVals[i], minorVals[i], moderateVals[i], majorVals[i]) * 10.0f + 0.5f) / 10.0f;
      if (currentDepth[i] < 1.0) {
	currentDepth[i] = 0.0;
	}
    }
    sprintf(buffer, "%s/maxthres.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
    
  }
  
  if (outputThresP && (griddedOutputs & OG_MAXTHRESP) == OG_MAXTHRESP) {
    for (size_t i = 0; i < currentQ.size(); i++) {
      currentDepth[i] = floorf(ComputeThresValueP(maxGrid[i], actionVals[i], actionSDVals[i], minorVals[i], minorSDVals[i], moderateVals[i], moderateSDVals[i], majorVals[i], majorSDVals[i]) * 10.0f + 0.5f) / 10.0f;
    }
    sprintf(buffer, "%s/maxthresp.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    gridWriter.WriteGrid(&nodes, &currentDepth, buffer, false);
    
  }
  
  if (savePrecip) {
    sprintf(buffer, "%s/qpeaccum.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    gridWriter.WriteGrid(&nodes, &qpeAccum, buffer, false);
    sprintf(buffer, "%s/qpfaccum.%04i%02i%02i.%02i%02i%02i.tif", outputPath, ctWE->tm_year + 1900, ctWE->tm_mon + 1, ctWE->tm_mday, ctWE->tm_hour, ctWE->tm_min, ctWE->tm_sec);
    gridWriter.WriteGrid(&nodes, &qpfAccum, buffer, false);
  }

#if _OPENMP
    double simEndTime = omp_get_wtime();
    double timeDiff = simEndTime - simStartTime;
#endif

  sprintf(buffer, "%s/results.json", task->GetOutput());
  FILE *fp = fopen(buffer, "w");
  fprintf(fp, "{\n\"missingQPE\": %i,\n\"missingQPF\": %i", missingQPE, missingQPF);
#if _OPENMP
  fprintf(fp, ",\n\"runTimeSeconds\": %f", timeDiff);
#endif
  fprintf(fp, "\n%s", "}");
  fclose(fp);
  
}

void Simulator::SimulateLumped() {
  PrecipReader precipReader;
  PETReader petReader;
  char buffer[CONFIG_MAX_LEN*2];
  size_t tsIndex = 0;
  
  std::vector<float> SM;
  SM.resize(currentFF.size());
  
  
  // Initialize our model
  wbModel->InitializeModel(&lumpedNodes, &fullParamSettings, &paramGrids);
  
  // This is the temporal loop for each time step
  // Here we load the input forcings & actually run the model
  for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
    currentTimeText.UpdateName(currentTime.GetTM());
    printf("%s", currentTimeText.GetName());
    
    if (!preloadedForcings) {
      
      if (currentTimePrecip < currentTime) {
        currentTimePrecip.Increment(timeStepPrecip);
      }
      
      if (currentTimePET < currentTime) {
        currentTimePET.Increment(timeStepPET);
      }
      
      precipFile->UpdateName(currentTimePrecip.GetTM());
      petFile->UpdateName(currentTimePET.GetTM());
      
      sprintf(buffer, "%s/%s", precipSec->GetLoc(), precipFile->GetName());
      if (!precipReader.Read(buffer, precipSec->GetType(), &nodes, &currentPrecipSimu, precipConvert)) {
        printf(" Missing precip file(%s)... Assuming zeros.", buffer);
      }
      
      sprintf(buffer, "%s/%s", petSec->GetLoc(), petFile->GetName());
      if (!petReader.Read(buffer, petSec->GetType(), &nodes, &currentPETSimu, petConvert, petSec->IsTemperature(), (float)currentTime.GetTM()->tm_yday)) {
        printf(" Missing PET file(%s)... Assuming zeros.", buffer);
      }
      
    }
    
    
    if (!preloadedForcings) {
      gaugeMap.GaugeAverage(&nodes, &currentPrecipSimu, &avgPrecip);
      gaugeMap.GaugeAverage(&nodes, &currentPETSimu, &avgPET);
      
      wbModel->WaterBalance(timeStepHours, &avgPrecip, &avgPET, &currentFF, &currentSF, &SM);
    } else {
      wbModel->WaterBalance(timeStepHours, &(currentPrecipCali[tsIndex]), &(currentPETCali[tsIndex]), &currentFF, &currentSF, &SM);
    }
    // We only output after the warmup period is over
    if (warmEndTime <= currentTime) {
      
      // Write the output to file
      for (size_t i = 0; i < gauges->size(); i++) {
        GaugeConfigSection *gauge = gauges->at(i);
        if (gaugeOutputs[i]) {
          float discharge = (currentFF[gauge->GetGridNodeIndex()] + currentSF[gauge->GetGridNodeIndex()]) * nodes[gauge->GetGridNodeIndex()].area / 3.6;
          if (!preloadedForcings) {
            fprintf(gaugeOutputs[i], "%s,%.2f,%.2f,%.2f,%.2f\n", currentTimeText.GetName(), discharge, gauge->GetObserved(&currentTime), avgPrecip[i], avgPET[i]);
          } else {
            fprintf(gaugeOutputs[i], "%s,%.2f,%.2f,%.2f,%.2f\n", currentTimeText.GetName(), discharge, gauge->GetObserved(&currentTime), currentPrecipCali[tsIndex].at(i), currentPETCali[tsIndex].at(i));
          }
        }
      }
    }
    
    // All of our status messages are done for this timestep!
    printf("%s", "\n");
    tsIndex++;
  }
  
}

void Simulator::PreloadForcings(char *file, bool cali) {
  
  PrecipReader precipReader;
  PETReader petReader;
  TempReader tempReader;
  char buffer[CONFIG_MAX_LEN*2];
  std::vector<float> readVec;
  size_t tsIndex = 0, tsIndexWarm = 0;
  
  if (LoadSavedForcings(file, cali)) {
    // We found a saved forcing file that we loaded, woo!
    return;
  }
  
  // Initialize TempReader
  if (sModel) {
    tempReader.ReadDEM(tempSec->GetDEM());
  } else {
    tempReader.SetNullDEM();
  }
  
  currentTime = beginTime;
  for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
    if (currentTimePrecip < currentTime) {
      currentTimePrecip.Increment(timeStepPrecip);
    }
    
    if (currentTimePET < currentTime) {
      currentTimePET.Increment(timeStepPET);
    }
    
    if (sModel && currentTimeTemp < currentTime) {
      currentTimeTemp.Increment(timeStepTemp);
    }
    
    // CONT
    
    precipFile->UpdateName(currentTimePrecip.GetTM());
    petFile->UpdateName(currentTimePET.GetTM());
    if (sModel) {
      tempFile->UpdateName(currentTimeTemp.GetTM());
    }
    
    
    std::vector<float> *precipVec = &(currentPrecipCali[tsIndex]);
    std::vector<float> *petVec = &(currentPETCali[tsIndex]);
    std::vector<float> *tempVec = NULL;
    if (sModel) {
      tempVec = &(currentTempCali[tsIndex]);
    }
    if (!wbModel->IsLumped()) {
      precipVec->resize(nodes.size());
      petVec->resize(nodes.size());
      if (sModel) {
        tempVec->resize(nodes.size());
      }
    } else {
      precipVec->resize(gauges->size());
      petVec->resize(gauges->size());
      if (sModel) {
        tempVec->resize(gauges->size());
      }
    }
    
    if (wbModel->IsLumped()) {
      // We are a lumped model...
      // therefore only care about averages!
      //
      readVec.resize(nodes.size());
      std::vector<float> *vec, *vecPrev;
      
      sprintf(buffer, "%s/%s", precipSec->GetLoc(), precipFile->GetName());
      vec = &(currentPrecipCali[tsIndex]);
      if (tsIndex > 0) {
        vec = &(currentPrecipCali[tsIndex-1]);
      } else {
        vecPrev = NULL;
      }
      if (!precipReader.Read(buffer, precipSec->GetType(), &nodes, &readVec, precipConvert, vecPrev)) {
        NORMAL_LOGF("Missing precip file(%s)... Assuming zeros.\n", buffer);
      }
      gaugeMap.GaugeAverage(&nodes, &readVec, vec);
      
      sprintf(buffer, "%s/%s", petSec->GetLoc(), petFile->GetName());
      vec = &(currentPETCali[tsIndex]);
      if (tsIndex > 0) {
        vec = &(currentPETCali[tsIndex-1]);
      } else {
        vecPrev = NULL;
      }
      if (!petReader.Read(buffer, petSec->GetType(), &nodes, &readVec, petConvert, petSec->IsTemperature(), currentTime.GetTM()->tm_yday, vecPrev)) {
        NORMAL_LOGF("Missing PET file(%s)... Assuming zeros.\n", buffer);
      }
      gaugeMap.GaugeAverage(&nodes, &readVec, vec);
    } else {
      std::vector<float> *vec, *vecPrev;
      
      sprintf(buffer, "%s/%s", precipSec->GetLoc(), precipFile->GetName());
      vec = &(currentPrecipCali[tsIndex]);
      if (tsIndex > 0) {
        vecPrev = &(currentPrecipCali[tsIndex-1]);
      } else {
        vecPrev = NULL;
      }
      if (!precipReader.Read(buffer, precipSec->GetType(), &nodes, vec, precipConvert, vecPrev)) {
        NORMAL_LOGF("Missing precip file(%s)... Assuming zeros.\n", buffer);
      }
      
      sprintf(buffer, "%s/%s", petSec->GetLoc(), petFile->GetName());
      vec = &(currentPETCali[tsIndex]);
      if (tsIndex > 0) {
        vecPrev = &(currentPETCali[tsIndex-1]);
      } else {
        vecPrev = NULL;
      }
      if (!petReader.Read(buffer, petSec->GetType(), &nodes, vec, petConvert, petSec->IsTemperature(), currentTime.GetTM()->tm_yday, vecPrev)) {
        NORMAL_LOGF("Missing PET file(%s)... Assuming zeros.\n", buffer);
      }
      
      if (sModel) {
        sprintf(buffer, "%s/%s", tempSec->GetLoc(), tempFile->GetName());
        vec = &(currentTempCali[tsIndex]);
        if (tsIndex > 0) {
          vecPrev = &(currentTempCali[tsIndex-1]);
        } else {
          vecPrev = NULL;
        }
        if (!tempReader.Read(buffer, tempSec->GetType(), &nodes, vec, vecPrev)) {
          NORMAL_LOGF("Missing Temp file(%s)... Assuming zeros.\n", buffer);
        }
      }
      
    }
    
    if (cali && warmEndTime <= currentTime) {
      obsQ[tsIndexWarm] = caliGauge->GetObserved(&currentTime);
      tsIndexWarm++;
    }
    
    tsIndex++;
  }
  
  SaveForcings(file);
}

bool Simulator::LoadSavedForcings(char *file, bool cali) {
  gzFile filep = gzopen(file, "r");
  if (filep == NULL) {
    WARNING_LOGF("Failed to load preload file %s", file);
    return false;
  }
  time_t temp;
  if (gzread(filep, &temp, sizeof(time_t)) != sizeof(time_t) || !(temp == beginTime.currentTimeSec)) {
    gzclose(filep);
    WARNING_LOGF("Wrong beginning time for preload file %s, not loaded", file);
    return false;
  }
  if (gzread(filep, &temp, sizeof(time_t)) != sizeof(time_t) || !(temp == endTime.currentTimeSec)) {
    gzclose(filep);
    WARNING_LOGF("Wrong ending time for preload file %s, not loaded (%lu %lu)", file, temp, endTime.currentTimeSec);
    return false;
  }
  size_t steps;
  if (gzread(filep, &steps, sizeof(totalTimeSteps)) != sizeof(totalTimeSteps) || steps != totalTimeSteps) {
    gzclose(filep);
    WARNING_LOGF("Wrong number of timesteps for preload file %s, not loaded (%lu %lu)", file, steps, totalTimeSteps);
    return false;
  }
  
  INFO_LOGF("Loading saved binary forcing file, %s!", file);
  
  size_t numDataPoints;
  if (!wbModel->IsLumped()) {
    numDataPoints = nodes.size();
  } else {
    numDataPoints = gauges->size();
  }
  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    std::vector<float> *precipVec = &(currentPrecipCali[tsIndex]);
    std::vector<float> *petVec = &(currentPETCali[tsIndex]);
    std::vector<float> *tempVec = NULL;
    precipVec->resize(numDataPoints);
    petVec->resize(numDataPoints);
    gzread(filep, &(precipVec->at(0)), (unsigned int)(sizeof(float) * numDataPoints));
    gzread(filep, &(petVec->at(0)), (unsigned int)(sizeof(float) * numDataPoints));
    if (sModel) {
      tempVec = &(currentTempCali[tsIndex]);
      tempVec->resize(numDataPoints);
      gzread(filep, &(tempVec->at(0)), (unsigned int)(sizeof(float) * numDataPoints));
    }
  }
  
  gzclose(filep);
  
  if (!cali) {
    return true;
  }
  
  size_t tsIndexWarm = 0;
  currentTime = beginTime;
  for (currentTime.Increment(timeStep); currentTime <= endTime; currentTime.Increment(timeStep)) {
    if (warmEndTime <= currentTime) {
      obsQ[tsIndexWarm] = caliGauge->GetObserved(&currentTime);
      tsIndexWarm++;
    }
  }
  return true;
}

void Simulator::SaveForcings(char *file) {
  gzFile filep = gzopen(file, "w9");
  gzwrite(filep, &(beginTime.currentTimeSec), sizeof(time_t));
  gzwrite(filep, &(endTime.currentTimeSec), sizeof(time_t));
  gzwrite(filep, &totalTimeSteps, sizeof(totalTimeSteps));
  int numDataPoints;
  if (!wbModel->IsLumped()) {
    numDataPoints = (int)nodes.size();
  } else {
    numDataPoints = (int)gauges->size();
  }
  for (size_t tsIndex = 0; tsIndex < totalTimeSteps; tsIndex++) {
    std::vector<float> *precipVec = &(currentPrecipCali[tsIndex]);
    std::vector<float> *petVec = &(currentPETCali[tsIndex]);
    gzwrite(filep, &(precipVec->at(0)), sizeof(float)*numDataPoints);
    gzwrite(filep, &(petVec->at(0)), sizeof(float)*numDataPoints);
    if (sModel) {
      std::vector<float> *tempVec = &(currentTempCali[tsIndex]);
      gzwrite(filep, &(tempVec->at(0)), sizeof(float)*numDataPoints);
    }
  }
  gzclose(filep);
}

float Simulator::SimulateForCali(float *testParams) {
  
  WaterBalanceModel *runModel;
  RoutingModel *runRoutingModel;
  SnowModel *runSnowModel;
  std::vector<float> currentFFCali, currentSFCali, currentQCali, simQCali, SMCali, currentSWECali, currentPrecipSnow;
  TimeVar currentTimeCali;
  std::map<GaugeConfigSection *, float *> *currentWBParamSettings;
  std::map<GaugeConfigSection *, float *> *currentRParamSettings;
  std::map<GaugeConfigSection *, float *> *currentSParamSettings;
  float *currentWBParams, *currentRParams, *currentSParams;
#if _OPENMP
  int thread = omp_get_thread_num();
  runModel = caliWBModels[thread];
  runRoutingModel = caliRModels[thread];
  runSnowModel = caliSModels[thread];
  currentWBParamSettings = &(caliWBFullParamSettings[thread]);
  currentWBParams = caliWBCurrentParams[thread];
  currentRParamSettings = &(caliRFullParamSettings[thread]);
  currentRParams = caliRCurrentParams[thread];
  currentSParamSettings = &(caliSFullParamSettings[thread]);
  currentSParams = caliSCurrentParams[thread];
#else
  runModel = wbModel;
  runRoutingModel = rModel;
  runSnowModel = sModel;
  currentWBParamSettings = &fullParamSettings;
  currentRParamSettings = &fullParamSettingsRoute;
  currentSParamSettings = &fullParamSettingsSnow;
  currentWBParams = caliWBParams;
  currentRParams = caliRParams;
  currentSParams = caliSParams;
#endif
  
  memcpy(currentWBParams, testParams, sizeof(float)*numWBParams);
  memcpy(currentRParams, testParams+numWBParams, sizeof(float)*numRParams);
  memcpy(currentSParams, testParams+numWBParams+numRParams, sizeof(float)*numSParams);
  
  // Initialize our model
  if (!runModel->IsLumped()) {
    runModel->InitializeModel(&nodes, currentWBParamSettings, &paramGrids);
  } else {
    runModel->InitializeModel(&lumpedNodes, currentWBParamSettings, &paramGrids);
  }
  
  runRoutingModel->InitializeModel(&nodes, currentRParamSettings, &paramGridsRoute);
  
  if (runSnowModel) {
    runSnowModel->InitializeModel(&nodes, currentSParamSettings, &paramGridsSnow);
  }
  
  
  currentFFCali.resize(currentFF.size());
  currentSFCali.resize(currentFF.size());
  currentQCali.resize(currentFF.size());
  currentSWECali.resize(currentFF.size());
  currentPrecipSnow.resize(currentFF.size());
  SMCali.resize(currentFF.size());
  simQCali.resize(simQ.size());
  avgPrecip.resize(gauges->size());
  avgPET.resize(gauges->size());
  
  // This is the temporal loop for each time step
  // Here we actually run the model
  size_t tsIndex = 0, tsIndexWarm = 0;
  currentTimeCali = beginTime;
  
  for (currentTimeCali.Increment(timeStep); currentTimeCali <= endTime; currentTimeCali.Increment(timeStep)) {
    
    std::vector<float> *precipVec = &(currentPrecipCali[tsIndex]);
    std::vector<float> *petVec = &(currentPETCali[tsIndex]);
    
    if (runSnowModel) {
      std::vector<float> *tempVec = &(currentTempCali[tsIndex]);
      runSnowModel->SnowBalance((float)currentTimeCali.GetTM()->tm_yday, timeStepHours, precipVec, tempVec, &currentPrecipSnow, &currentSWECali);
      precipVec = &currentPrecipSnow;
    }
    /*if (tsIndex == 0) {
     gaugeMap.GaugeAverage(&nodes, precipVec, &avgPrecip);
     gaugeMap.GaugeAverage(&nodes, petVec, &avgPET);
     printf("%f %f\n", precipVec->at(300), petVec->at(300));
     }*/
    runModel->WaterBalance(timeStepHours, precipVec, petVec, &currentFFCali, &currentSFCali, &SMCali);
    
    runRoutingModel->Route(timeStepHours, &currentFFCali, &currentSFCali, &currentQCali);
    
    if (warmEndTime <= currentTimeCali) {
      simQCali[tsIndexWarm] = currentQCali[caliGauge->GetGridNodeIndex()];
      tsIndexWarm++;
    }
    
    tsIndex++;
  }
  float skill = CalcObjFunc(&obsQ, &simQCali, objectiveFunc);
#if _OPENMP
  //printf("%i: %f %f\n", thread, skill, rP[0]);
  /*if (skill < -2000.0) {
   for (unsigned int i = 0; i < tsIndexWarm; i++) {
   printf("dump %i: %f %f\n", thread, obsQ[i], simQCali[i]);
   }
   }*/
#else
  //printf("%f\n", skill);
#endif
  return skill;
  //return CalcObjFunc(&obsQ, &simQCali, objectiveFunc);
}

float *Simulator::SimulateForCaliTS(float *testParams) {
  
  WaterBalanceModel *runModel;
  std::vector<float> currentFFCali, currentSFCali, SMCali;
  float *simQCali;
  TimeVar currentTimeCali;
  std::map<GaugeConfigSection *, float *> *currentParamSettings;
  float *currentParams;
#if 0 //def _OPENMP
  int thread = omp_get_thread_num();
  runModel = caliModels[thread];
  currentParamSettings = &(caliFullParamSettings[thread]);
  currentParams = caliCurrentParams[thread];
#else
  runModel = wbModel;
  currentParamSettings = &fullParamSettings;
  currentParams = caliWBParams;
#endif
  
  memcpy(currentParams, testParams, sizeof(float)*numWBParams);
  
  // Initialize our model
  if (!runModel->IsLumped()) {
    runModel->InitializeModel(&nodes, currentParamSettings, &paramGrids);
  } else {
    runModel->InitializeModel(&lumpedNodes, currentParamSettings, &paramGrids);
  }
  
  currentFFCali.resize(currentFF.size());
  SMCali.resize(currentFF.size());
  simQCali = new float[simQ.size()];
  
  // This is the temporal loop for each time step
  // Here we actually run the model
  size_t tsIndex = 0, tsIndexWarm = 0;
  currentTimeCali = beginTime;
  
  for (currentTimeCali.Increment(timeStep); currentTimeCali <= endTime; currentTimeCali.Increment(timeStep)) {
    
    std::vector<float> *precipVec = &(currentPrecipCali[tsIndex]);
    std::vector<float> *petVec = &(currentPETCali[tsIndex]);
    
    runModel->WaterBalance(timeStepHours, precipVec, petVec, &currentFFCali, &currentSFCali, &SMCali);
    if (warmEndTime <= currentTimeCali) {
      if (!runModel->IsLumped()) {
        simQCali[tsIndexWarm] = currentFFCali[caliGauge->GetGridNodeIndex()];
      } else {
        simQCali[tsIndexWarm] = currentFFCali[caliGaugeIndex];
      }
      tsIndexWarm++;
    }
    
    tsIndex++;
  }
  
  return simQCali;
}

float *Simulator::GetObsTS() {
  float *obsData = new float[obsQ.size()];
  
  for (size_t i = 0; i < obsQ.size(); i++) {
    obsData[i] = obsQ[i];
  }
  
  return obsData;
}

bool Simulator::InitializeGridParams(TaskConfigSection *task) {
  int numParams = numModelParams[task->GetModel()];
  std::vector<std::string> *vecGrids = task->GetParamsSec()->GetParamGrids();
  
  paramGrids.resize(numParams);
  
  for (int i = 0; i < numParams; i++) {
    std::string *file = &(vecGrids->at(i));
    if (file->length() == 0) {
      paramGrids[i] = NULL;
    } else {
      paramGrids[i] = ReadFloatTifGrid(file->c_str());
      if (!paramGrids[i]) {
        ERROR_LOGF("Failed to load water balance parameter grid %s\n", file->c_str());
        return false;
      }
    }
  }
 
	if (task->GetRouting() != ROUTE_QTY) { 
  	int numRParams = numRouteParams[task->GetRouting()];
  	std::vector<std::string> *vecRouteGrids = task->GetRoutingParamsSec()->GetParamGrids();
  
  	paramGridsRoute.resize(numRParams);
  
  	for (int i = 0; i < numRParams; i++) {
    	std::string *file = &(vecRouteGrids->at(i));
    	if (file->length() == 0) {
      	paramGridsRoute[i] = NULL;
    	} else {
      	paramGridsRoute[i] = ReadFloatTifGrid(file->c_str());
      	if (!paramGridsRoute[i]) {
        	ERROR_LOGF("Failed to load routing parameter grid %s\n", file->c_str());
        	return false;
      	}
    	}
  	}
	}
  
  if (task->GetSnow() != SNOW_QTY) {
    int numSParams = numSnowParams[task->GetSnow()];
    std::vector<std::string> *vecSnowGrids = task->GetSnowParamsSec()->GetParamGrids();
    
    paramGridsSnow.resize(numSParams);
    
    for (int i = 0; i < numSParams; i++) {
      std::string *file = &(vecSnowGrids->at(i));
      if (file->length() == 0) {
        paramGridsSnow[i] = NULL;
      } else {
        paramGridsSnow[i] = ReadFloatTifGrid(file->c_str());
        if (!paramGridsSnow[i]) {
          ERROR_LOGF("Failed to load snow parameter grid %s\n", file->c_str());
          return false;
        }
      }
    }
    
  }
  
  if (task->GetInundation() != INUNDATION_QTY) {
    int numIParams = numInundationParams[task->GetInundation()];
    std::vector<std::string> *vecInundationGrids = task->GetInundationParamsSec()->GetParamGrids();
    
    paramGridsInundation.resize(numIParams);
    
    for (int i = 0; i < numIParams; i++) {
      std::string *file = &(vecInundationGrids->at(i));
      if (file->length() == 0) {
        paramGridsInundation[i] = NULL;
      } else {
        paramGridsInundation[i] = ReadFloatTifGrid(file->c_str());
        if (!paramGridsInundation[i]) {
          ERROR_LOGF("Failed to load inundation parameter grid %s\n", file->c_str());
          return false;
        }
      }
    }
    
  }
  
  return true;
  
}

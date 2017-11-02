#include "GaugeMap.h"
#include <cstdio>

void GaugeMap::Initialize(std::vector<GaugeConfigSection *> *newGauges) {
  // Copy the list of gauges over to internal storage
  gauges = (*newGauges);

  size_t countGauges = gauges.size();

  // Resize the outer vector in the tree that contains all of the interior
  // gauges
  gaugeTree.resize(countGauges);

  // Initialize the map that contains the index into a vector for each
  // GaugeConfigSection *
  for (size_t i = 0; i < countGauges; i++) {
    gaugeMap[gauges[i]] = i;
  }

  // Initialize storage for the partial values contributing to each gauge
  partialVal.resize(countGauges);
  partialArea.resize(countGauges);
}

void GaugeMap::AddUpstreamGauge(GaugeConfigSection *downStream,
                                GaugeConfigSection *upStream) {
  size_t countGauges = gauges.size();
  for (size_t i = 0; i < countGauges; i++) {
    if (gauges[i] == downStream) {
      printf("%s is upstream(direct) of %s\n", upStream->GetName(),
             downStream->GetName());
      gaugeTree[i].push_back(upStream);
    }
  }

  for (size_t i = 0; i < countGauges; i++) {
    std::vector<GaugeConfigSection *> *intGauges = &(gaugeTree[i]);
    for (size_t j = 0; j < intGauges->size(); j++) {
      if (intGauges->at(j) == downStream) {
        printf("%s is upstream(indirect) of %s\n", upStream->GetName(),
               gauges[j]->GetName());
        intGauges->push_back(upStream);
      }
    }
  }
}

void GaugeMap::GaugeAverage(std::vector<GridNode> *nodes,
                            std::vector<float> *currentValue,
                            std::vector<float> *gaugeAvg) {
  size_t countGauges = gauges.size();
  size_t countNodes = nodes->size();

  // Zero out the partial vectors
  for (size_t i = 0; i < countGauges; i++) {
    partialVal[i] = 0;
    partialArea[i] = 0;
  }

  // Add up contributions to each gauge
  for (size_t i = 0; i < countNodes; i++) {
    GridNode *node = &((*nodes)[i]);
    size_t gaugeIndex = gaugeMap[node->gauge];
    partialVal[gaugeIndex] += ((*currentValue)[i] * node->area);
    partialArea[gaugeIndex] += node->area;
  }

  for (size_t i = 0; i < countGauges; i++) {
    float totalVal = 0;
    float totalArea = 0;

    totalVal += partialVal[i];
    totalArea += partialArea[i];

    std::vector<GaugeConfigSection *> *intGauges = &(gaugeTree[i]);
    for (size_t j = 0; j < intGauges->size(); j++) {
      size_t gaugeIndex = gaugeMap[intGauges->at(j)];
      totalVal += partialVal[gaugeIndex];
      totalArea += partialArea[gaugeIndex];
    }

    gaugeAvg->at(i) = (totalVal / totalArea);
  }
}

void GaugeMap::GetGaugeArea(std::vector<GridNode> *nodes,
                            std::vector<float> *gaugeArea) {

  size_t countGauges = gauges.size();
  size_t countNodes = nodes->size();

  // Zero out the partial vectors
  for (size_t i = 0; i < countGauges; i++) {
    partialArea[i] = 0;
  }

  // Add up contributions to each gauge
  for (size_t i = 0; i < countNodes; i++) {
    GridNode *node = &((*nodes)[i]);
    size_t gaugeIndex = gaugeMap[node->gauge];
    partialArea[gaugeIndex] += node->area;
  }

  for (size_t i = 0; i < countGauges; i++) {
    float totalArea = 0;

    totalArea += partialArea[i];

    std::vector<GaugeConfigSection *> *intGauges = &(gaugeTree[i]);
    for (size_t j = 0; j < intGauges->size(); j++) {
      size_t gaugeIndex = gaugeMap[intGauges->at(j)];
      totalArea += partialArea[gaugeIndex];
    }

    gaugeArea->at(i) = totalArea;
  }
}

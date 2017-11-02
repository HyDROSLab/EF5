#ifndef TIME_SERIES_H
#define TIME_SERIES_H

#include "TimeVar.h"
#include <cstdio>
#include <vector>

struct TSDataPoint {
  TimeVar time;
  float value;
};

class TimeSeries {

public:
  void LoadTimeSeries(char *file);
  void PutValueAtTime(char *timeBuffer, float dataValue);
  float GetValueAtTime(TimeVar *wantTime);
  float GetValueNearTime(TimeVar *wantTime, time_t diff);
  size_t GetNumberOfObs() { return timeSeries.size(); }

private:
  std::vector<TSDataPoint *> timeSeries;
  size_t lastIndex;
};

#endif

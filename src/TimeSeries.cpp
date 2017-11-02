#include "TimeSeries.h"
#include "Defines.h"
#include "Messages.h"
#include <cstdio>
#include <limits>

void TimeSeries::LoadTimeSeries(char *file) {
  FILE *tsFile;

  tsFile = fopen(file, "rb");
  if (tsFile == NULL) {
    WARNING_LOGF("Failed to open time series file %s", file);
    return;
  }

  // printf("struct size is %i\n", sizeof(TSDataPoint));

  // Get number of file lines
  /*int fileLines = 0, temp;
  while ( (temp = fgetc(tsFile)) != EOF) {
          fileLines += (temp == 10);
  }
  fseek(tsFile, 0, SEEK_SET);

  for (int i = 0; i < fileLines; i++) {*/
  while (!feof(tsFile)) {
    char buffer[CONFIG_MAX_LEN];
    float dataValue;
    if (fscanf(tsFile, "%[^,],%f ", &(buffer[0]), &dataValue) == 2) {
      TSDataPoint *pt = new TSDataPoint;
      pt->time.LoadTimeExcel(buffer);
      pt->value = dataValue;
      timeSeries.push_back(pt);
    } else {
      // Skip past this line because it is the wrong format
      char *output = fgets(buffer, CONFIG_MAX_LEN, tsFile);
      (void)output;
    }
  }

  fclose(tsFile);
  lastIndex = 0;
}

void TimeSeries::PutValueAtTime(char *timeBuffer, float dataValue) {
  TSDataPoint *pt = new TSDataPoint;
  pt->time.LoadTimeExcel(timeBuffer);
  pt->value = dataValue;
  timeSeries.push_back(pt);
}

float TimeSeries::GetValueAtTime(TimeVar *wantTime) {
  for (size_t i = lastIndex; i < timeSeries.size(); i++) {
    if (timeSeries[i]->time == (*wantTime)) {
      lastIndex = i;
      return timeSeries[i]->value;
    }
    if ((*wantTime) < timeSeries[i]->time) {
      return std::numeric_limits<float>::quiet_NaN();
    }
  }
  /*for (std::vector<TSDataPoint>::iterator itr = timeSeries.begin(); itr !=
  timeSeries.end(); itr++) { TSDataPoint *pt = &(*itr); if (pt->time ==
  (*wantTime)) { return pt->value;
          }
  }*/

  return std::numeric_limits<float>::quiet_NaN();
}

float TimeSeries::GetValueNearTime(TimeVar *wantTime, time_t diff) {
  for (size_t i = lastIndex; i < timeSeries.size(); i++) {
    if (timeSeries[i]->time == (*wantTime)) {
      lastIndex = i;
      return timeSeries[i]->value;
    } else if ((*wantTime) < timeSeries[i]->time) {
      if ((timeSeries[i]->time.currentTimeSec - wantTime->currentTimeSec) <
          diff) {
        return timeSeries[i]->value;
      }
      return std::numeric_limits<float>::quiet_NaN();
    }
  }

  return std::numeric_limits<float>::quiet_NaN();
}

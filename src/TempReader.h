#ifndef TEMP_READER_H
#define TEMP_READER_H

#include "BasicGrids.h"
#include "Defines.h"
#include "TempType.h"
#include <vector>

class TempReader {
public:
  bool Read(char *file, SUPPORTED_TEMP_TYPES type, std::vector<GridNode> *nodes,
            std::vector<float> *currentTemp,
            std::vector<float> *prevTemp = NULL, bool hasF = false);
  void ReadDEM(char *file);
  void SetNullDEM() { tempDEM = NULL; }

private:
  char lastTempFile[CONFIG_MAX_LEN * 2];
  FloatGrid *tempDEM;
};

#endif

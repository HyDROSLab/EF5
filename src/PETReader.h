#ifndef PET_READER_H
#define PET_READER_H

#include "BasicGrids.h"
#include "Defines.h"
#include "PETType.h"
#include <vector>

class PETReader {
public:
  bool Read(char *file, SUPPORTED_PET_TYPES type, std::vector<GridNode> *nodes,
            std::vector<float> *currentPET, float petConvert, bool isTemp,
            float jday, std::vector<float> *prevPET = NULL);

private:
  char lastPETFile[CONFIG_MAX_LEN * 2];
};

#endif

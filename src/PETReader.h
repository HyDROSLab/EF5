#ifndef PET_READER_H
#define PET_READER_H

#include <vector>
#include "Defines.h"
#include "BasicGrids.h"
#include "PETType.h"

class PETReader {
	public:
		bool Read(char *file, SUPPORTED_PET_TYPES type, std::vector<GridNode> *nodes, std::vector<float> *currentPET, float petConvert, bool isTemp, float jday, std::vector<float> *prevPET = NULL);

	private:
		char lastPETFile[CONFIG_MAX_LEN*2];

};

#endif

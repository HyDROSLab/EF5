#ifndef PRECIP_READER_H
#define PRECIP_READER_H

#include <vector>
#include "Defines.h"
#include "BasicGrids.h"
#include "PrecipType.h"

class PrecipReader {
	public:
		bool Read(char *file, SUPPORTED_PRECIP_TYPES type, std::vector<GridNode> *nodes, std::vector<float> *currentPrecip, float precipConvert, std::vector<float> *prevPrecip = NULL, bool hasQPF = false);

	private:
		char lastPrecipFile[CONFIG_MAX_LEN*2];

};

#endif

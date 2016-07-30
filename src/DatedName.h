#ifndef DATED_NAME_H
#define DATED_NAME_H

#include <time.h>
#include "Defines.h"
#include "TimeUnit.h"

extern const char *datedNameStrings[];
extern const int datePartLen[];

class DatedName {

	public:
		bool ProcessName(TimeUnit *freq);
		bool ProcessNameLoose(TimeUnit *freq);
		void SetNameStr(const char *nameStr);
		void UpdateName(tm *ptm);
		char *GetName() { return inUseName; }

	private:
		SUPPORTED_TIME_UNITS resolution;
		char inUseName[CONFIG_MAX_LEN];
		char *timeParts[TIME_UNIT_QTY];
		bool inUseTimeParts[TIME_UNIT_QTY];
		
	
};

#endif

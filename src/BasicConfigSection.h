#ifndef CONFIG_BASIC_SECTION
#define CONFIG_BASIC_SECTION

#include "Defines.h"
#include "ConfigSection.h"
#include "Projection.h"

class BasicConfigSection : public ConfigSection {

	public:
		BasicConfigSection();
		~BasicConfigSection();

		char *GetDEM();
		char *GetDDM();
		char *GetFAM();
		PROJECTIONS GetProjection();
		bool IsESRIDDM() { return esriDDM; }
		bool IsSelfFAM() { return selfFAM; }		
		CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
		CONFIG_SEC_RET ValidateSection();

	private:
		bool DEMSet, DDMSet, FAMSet, projectionSet, esriDDMSet, selfFAMSet;
		bool esriDDM, selfFAM;	
		char DEM[CONFIG_MAX_LEN];
		char DDM[CONFIG_MAX_LEN];
		char FAM[CONFIG_MAX_LEN];
		PROJECTIONS projection;
		
		
};

extern BasicConfigSection *g_basicConfig;

#endif

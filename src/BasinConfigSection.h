#ifndef CONFIG_BASIN_SECTION_H
#define CONFIG_BASIN_SECTION_H

#include <map>
#include <vector>
#include <string>
#include "Defines.h"
#include "ConfigSection.h"
#include "GaugeConfigSection.h"

class BasinConfigSection : public ConfigSection {

	public:
		BasinConfigSection(char *newName);
		~BasinConfigSection();

		char *GetName() { return name; }	
		std::vector<GaugeConfigSection *> *GetGauges() { return &gauges; }	
		CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
		CONFIG_SEC_RET ValidateSection();

		static bool IsDuplicate(char *name);

	private:
		bool IsDuplicateGauge(GaugeConfigSection *gauge);
		char name[CONFIG_MAX_LEN];
		std::vector<GaugeConfigSection *> gauges;	
};

extern std::map<std::string, BasinConfigSection *> g_basinConfigs;

#endif

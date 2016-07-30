#ifndef CONFIG_EXECUTE_SECTION_H
#define CONFIG_EXECUTE_SECTION_H

#include <vector>
#include <string>
#include "Defines.h"
#include "ConfigSection.h"
#include "EnsTaskConfigSection.h"
#include "TaskConfigSection.h"


class ExecuteConfigSection : public ConfigSection {

	public:
		ExecuteConfigSection();
		~ExecuteConfigSection();
		
		CONFIG_SEC_RET ProcessKeyValue(char *name, char *value);
		CONFIG_SEC_RET ValidateSection();

		std::vector<TaskConfigSection *> *GetTasks();
		std::vector<EnsTaskConfigSection *> *GetEnsTasks();
	private:
		std::vector<TaskConfigSection *> tasks;
		std::vector<EnsTaskConfigSection *> ensTasks;
};

extern ExecuteConfigSection *g_executeConfig;

#endif

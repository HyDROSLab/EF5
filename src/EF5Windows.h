#ifndef EF5WINDOWS_H
#define EF5WINDOWS_H

enum PROGRAM_RESULTS { EF5_ERROR_SUCCESS = 0, EF5_ERROR_INVALIDCONF };

enum CONSOLEMESSAGETYPE {
  NORMAL = 0,
  INFORMATION = 1,
  WARNING = 2,
  FATAL = 3,
};

void addConsoleText(CONSOLEMESSAGETYPE type, const char *szFmt, ...);
void setTimestep(const char *szTime);
void setIteration(int current);

#endif

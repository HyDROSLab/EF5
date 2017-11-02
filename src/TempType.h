#ifndef TEMP_TYPE_H
#define TEMP_TYPE_H

enum SUPPORTED_TEMP_TYPES {
  TEMP_ASCII,
  TEMP_TIF,
  TEMP_TYPE_QTY,
};

extern const char *tempTypeStrings[];

class TempType {

public:
  SUPPORTED_TEMP_TYPES GetType();
  SUPPORTED_TEMP_TYPES ParseType(char *typeStr);

private:
  SUPPORTED_TEMP_TYPES type;
};

#endif

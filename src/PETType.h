#ifndef PET_TYPE_H
#define PET_TYPE_H

enum SUPPORTED_PET_TYPES {
  PET_ASCII,
  PET_BIF,
  PET_TIF,
  PET_TYPE_QTY,
};

extern const char *petTypeStrings[];

class PETType {

public:
  SUPPORTED_PET_TYPES GetType();
  SUPPORTED_PET_TYPES ParseType(char *typeStr);
  const char *GetTypes();

private:
  SUPPORTED_PET_TYPES type;
};

#endif

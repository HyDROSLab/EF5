#ifndef PRECIP_TYPE_H
#define PRECIP_TYPE_H

enum SUPPORTED_PRECIP_TYPES {
  PRECIP_ASCII,
  PRECIP_MRMS,
  PRECIP_TRMMRT,
  PRECIP_TRMMV7,
  PRECIP_BIF,
  PRECIP_TIF,
  PRECIP_TYPE_QTY,
};

extern const char *precipTypeStrings[];

class PrecipType {

public:
  SUPPORTED_PRECIP_TYPES GetType();
  SUPPORTED_PRECIP_TYPES ParseType(char *typeStr);
  const char *GetTypes();

private:
  SUPPORTED_PRECIP_TYPES type;
};

#endif

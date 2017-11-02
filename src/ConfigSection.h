#ifndef CONFIG_SECTION_H
#define CONFIG_SECTION_H

enum CONFIG_SEC_RET {
  VALID_RESULT,
  INVALID_RESULT,
};

class ConfigSection {

public:
  virtual CONFIG_SEC_RET ProcessKeyValue(char *name, char *value) = 0;
  virtual CONFIG_SEC_RET ValidateSection() = 0;
};

#endif

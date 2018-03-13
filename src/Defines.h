#ifndef DEFINES_H
#define DEFINES_H

#define EF5_VERSION "1.2.3"

#define CONFIG_MAX_LEN 256

#define TOLOWER(x)                                                             \
  for (char *s = x; *s; s++)                                                   \
    *s = tolower(*s);
#define TORADIANS(x) (x) / 180 * 3.1415926
#define TODEGREES(x) (x) / 3.1415926 * 180
#define PI 3.1415926

// This is here because both grids & projections need to know about it!
/*
enum FLOW_DIR {
        FLOW_NORTH = 1,
        FLOW_NORTHEAST,
        FLOW_EAST,
        FLOW_SOUTHEAST,
        FLOW_SOUTH,
        FLOW_SOUTHWEST,
        FLOW_WEST,
        FLOW_NORTHWEST,
        FLOW_QTY,
};*/

enum FLOW_DIR {
  FLOW_EAST = 1,
  FLOW_NORTHEAST,
  FLOW_NORTH,
  FLOW_NORTHWEST,
  FLOW_WEST,
  FLOW_SOUTHWEST,
  FLOW_SOUTH,
  FLOW_SOUTHEAST,
  FLOW_QTY,
};

#endif
